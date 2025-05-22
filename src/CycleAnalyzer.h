#ifndef CYCLE_ANALYZER_H
#define CYCLE_ANALYZER_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>

class CycleAnalyzer {
public:
  struct Result {
    unsigned long periodMs;
    int16_t minAdc;
    int16_t maxAdc;
    float vMin;
    float vMax;
    bool timeout;
    bool ready;
  };

  CycleAnalyzer(uint8_t signalPin, uint8_t adsChannel, unsigned long timeoutMs, Adafruit_ADS1115& adsRef);

  void begin();
  void start();
  void update();
  bool isReady() const;
  Result getResult();

private:
  enum State {
    WAITING_FIRST_FALLING,
    WAITING_RISING,
    WAITING_SECOND_FALLING
  };

  uint8_t signalPin;
  uint8_t channel;
  unsigned long timeout;
  Adafruit_ADS1115& ads;

  bool isRunning;
  unsigned long t0;
  unsigned long waitingStart;
  int16_t minVal;
  int16_t maxVal;
  int prevSignalState;

  Result result;
  State state;

  static constexpr unsigned long debounceDelayMs = 3;
  unsigned long fallingDebounceStart;
  unsigned long risingDebounceStart;

  void reset();
  void finalize(unsigned long period, bool isTimeout);
  void changeStateToWaitingFirstFalling();
  void changeStateToWaitingRising();
  void changeStateToWaitingSecondFalling();
  bool detectEdge(int prev, int curr, int from, int to, unsigned long now, unsigned long& lastTriggerTime);
};

#endif
