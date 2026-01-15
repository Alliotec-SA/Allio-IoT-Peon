#ifndef SOFTTIMER_H
#define SOFTTIMER_H

#include <Arduino.h>

class SoftTimer {
private:
    unsigned long lastTime;   // Last time the timer was checked
    unsigned long interval;   // Interval in milliseconds
    bool oneShotFlag;         // Flag for one-shot timers

public:
    // Constructor: interval in milliseconds
    SoftTimer(unsigned long ms = 1000);

    // Set / reset interval
    void setInterval(unsigned long ms);

    void start();

    // Periodic timer: returns true if interval has elapsed
    bool elapsed();

    // One-shot timer: returns true only once when interval reached
    bool expired();

    // Reset timer manually
    void reset();
};

#endif
