#ifndef SETTINGS_H
#define SETTINGS_H

#define SERIAL_BAUD_RATE 115200
#define PIN_SIGNAL 14
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