#include "CycleAnalyzer.h"

CycleAnalyzer::CycleAnalyzer(uint8_t signalPin, uint8_t adsChannel, unsigned long timeoutMs, Adafruit_ADS1115& adsRef)
  : signalPin(signalPin), channel(adsChannel), timeout(timeoutMs), ads(adsRef) {}

void CycleAnalyzer::begin() {
  pinMode(signalPin, INPUT);
  prevSignalState = HIGH;
  reset();
  isRunning = false;
}

void CycleAnalyzer::start() {
  reset();
  prevSignalState = digitalRead(signalPin);
  isRunning = true;
  changeStateToWaitingFirstFalling();
}

void CycleAnalyzer::update() {
  if (!isRunning || result.ready) return;

  unsigned long now = millis();
  int currentSignal = digitalRead(signalPin);

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

    case WAITING_SECOND_FALLING: {
      int16_t val = ads.readADC_SingleEnded(channel);
      if (val < minVal) minVal = val;
      if (val > maxVal) maxVal = val;

      if (fallingEdge) {
        finalize(now - t0, false);
      } else if ((now - t0) > timeout) {
        finalize(0, true);
      }
      break;
    }
  }
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
  isRunning = false;
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
