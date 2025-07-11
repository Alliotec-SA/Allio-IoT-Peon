#ifndef Fuctions_H
#define Fuctions_H
#include <Arduino.h>
#include "Settings.h"

float getVoltage(uint8_t channel);

float getSignalVp(float voltage);

float getOwnBatteryVoltage();
float getBatteryVoltage();
float getSolarPannelVoltage();
void testBoardVoltageElement(Stream &port);

void sendJsonPost(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec);



#endif