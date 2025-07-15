#ifndef SETTINGS_H
#define SETTINGS_H

#define LORAWAN_RESPONSE_BUFFER 128
#define RTC_ADDR 65  // safe address range is 64–127
#define READING_TRIES 3
#define MAX_TIME_TO_START_SETUP_IN_SECONDS 20 
#define UPDATE_TIME_IN_HOURS 1
#define INTERNAL_WAKEUP_TO_CHECK_UPDATE_TIME_IN_MINUTES 30

#define SERIAL_BAUD_RATE 115200
#define PIN_SIGNAL 14
#define PIN_DRST 12
#define PIN_WKP 16
#define PIN_SETTINGS_MODE 13
#define SIGNAL_FACTOR 2801
#define SIGNAL_TIMEOUT 6000
#define CHANNEL_SIGNAL 3
#define CHANNEL_BATTERY 1
#define CHANNEL_PANEL 2
#define CHANNEL_OWN_BATTERY 0
#define OWN_BATTERY_FACTOR 11.11 //(1)/(10/110)
#define BATTERY_FACTOR 11.11 //(1)/(10/110)
#define SOLAR_FACTOR 11.11 //(1)/(10/110)
#define UPDATE_TIME 60000



#include <Arduino.h>

struct LastResult {
    unsigned long signalPeriod;
    bool timeout;
    bool ready;
    unsigned long lastChecked;
    uint8_t batteryPercent;
    float batteryVoltage;
    float solarVoltage;
    float signalVoltage;
    float battery;
  };

#endif