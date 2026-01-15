#include "SoftTimer.h"

SoftTimer::SoftTimer(unsigned long ms) {
    interval = ms;
    lastTime = 0;
    oneShotFlag = false;
}

void SoftTimer::start() {
    lastTime = millis();
}

void SoftTimer::setInterval(unsigned long ms) {
    interval = ms;
}

bool SoftTimer::elapsed() {
    unsigned long now = millis();
    if (now - lastTime >= interval) {
        lastTime = now;
        return true;
    }
    return false;
}

bool SoftTimer::expired() {
    if (!oneShotFlag && (long)(millis() - (lastTime + interval)) >= 0) {
        oneShotFlag = true;
        lastTime = millis(); // optional: restart timer
        return true;
    }
    return false;
}

void SoftTimer::reset() {
    lastTime = millis();
    oneShotFlag = false;
}
