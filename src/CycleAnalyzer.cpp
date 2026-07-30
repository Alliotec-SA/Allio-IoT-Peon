#include "CycleAnalyzer.h"
#include "Settings.h"

CycleAnalyzer::CycleAnalyzer(uint8_t signalPin, uint8_t adsChannel, unsigned long timeoutMs, Adafruit_ADS1115& adsRef)
  : signalPin(signalPin), channel(adsChannel), timeout(timeoutMs), ads(adsRef) {}

void CycleAnalyzer::begin() {
  pinMode(signalPin, INPUT);
  prevSignalState = HIGH;
  reset();
  isRunning = false;
  result.readSecuence = 0;
}

void CycleAnalyzer::start() {
  reset();
  prevSignalState = digitalRead(signalPin);
  isRunning = true;
  changeStateToWaitingFirstFalling();
}

void CycleAnalyzer::cancel() {
  reset();
  isRunning = false;
}

void CycleAnalyzer::update() {
  if (!isRunning || result.ready) return;

  unsigned long now = millis();
  int currentSignal = digitalRead(signalPin);
  readAdcValue();

  bool fallingEdge = detectEdge(prevSignalState, currentSignal, HIGH, LOW, now, fallingDebounceStart);
  bool risingEdge  = detectEdge(prevSignalState, currentSignal, LOW, HIGH, now, risingDebounceStart);
  prevSignalState = currentSignal;

  switch (state) {
    case WAITING_FIRST_FALLING:
      if (fallingEdge) {
        t0 = now;
        changeStateToWaitingRising();
      } else if ((now - waitingStart) > timeout) {
        finalize(0, true);
      }
      break;

    case WAITING_RISING:
      if (risingEdge) {
        changeStateToWaitingSecondFalling();
      } else if ((now - t0) > timeout) {
        finalize(0, true);
      }
      break;

    case WAITING_SECOND_FALLING: 
      if (fallingEdge) {
        finalize(now - t0, false);
      } else if ((now - t0) > timeout) {
        finalize(0, true);
      }
      break;
  }
}

/** Bounded replacement for readADC_SingleEnded, whose wait loop has no timeout and spins forever
 *  if the I2C bus locks up. Polled without yield() on purpose: this runs inside the edge-detection
 *  loop, and handing control to the SDK here would stretch the sampling period unpredictably. */
bool CycleAnalyzer::readAdcCounts(int16_t& out) {
  ads.startADCReading(MUX_BY_CHANNEL[channel], /*continuous=*/false);
  const unsigned long start = millis();
  while (!ads.conversionComplete()) {
    if (millis() - start > ADC_READ_TIMEOUT_MS) {
      return false;
    }
  }
  out = ads.getLastConversionResults();
  return true;
}

void CycleAnalyzer::readAdcValue() {
  int16_t val;
  if (!readAdcCounts(val)) {
    // Drop the sample instead of feeding min/max: a failed I2C read leaves the driver's buffer
    // untouched, so the value returned would be stale bytes rather than a measurement.
    adcFailures++;
    return;
  }
  if (val < minVal) minVal = val;
  if (val > maxVal) maxVal = val;
}

bool CycleAnalyzer::isReady() const {
  return result.ready;
}

bool CycleAnalyzer::isAnalyzerRunning() {
  return isRunning;
}

CycleAnalyzer::Result CycleAnalyzer::getResult() {
  result.ready = false;
  return result;
}

void CycleAnalyzer::reset() {
  minVal = 32767;
  maxVal = -32768;
  adcFailures = 0;
  result.ready = false;
  result.timeout = false;
}

void CycleAnalyzer::finalize(unsigned long period, bool isTimeout) {
  result.periodMs = period;
  result.minAdc = minVal;
  result.maxAdc = maxVal;
  result.vMin = ads.computeVolts(minVal);
  result.vMax = ads.computeVolts(maxVal);
  result.timeout = isTimeout;
  result.ready = true;
  result.readSecuence = 0;
  isRunning = false;
  // Reported here rather than inside update(): printing in the sampling loop would stretch the
  // period and cost the very edges this class exists to catch.
  if (adcFailures > 0) {
    SerialDebug.print("ADC samples dropped this attempt: ");
    SerialDebug.println(adcFailures);
  }
}

void CycleAnalyzer::changeStateToWaitingFirstFalling() {
  state = WAITING_FIRST_FALLING;
  waitingStart = millis();
}

void CycleAnalyzer::changeStateToWaitingRising() {
  state = WAITING_RISING;
  waitingStart = millis();
}

void CycleAnalyzer::changeStateToWaitingSecondFalling() {
  state = WAITING_SECOND_FALLING;
}

bool CycleAnalyzer::detectEdge(int prev, int curr, int from, int to,
                               unsigned long now, unsigned long& lastTriggerTime) {
  if (prev == from && curr == to) {
    if ((now - lastTriggerTime) >= debounceDelayMs) {
      lastTriggerTime = now;
      return true;
    }
  }
  return false;
}
