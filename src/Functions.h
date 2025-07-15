#ifndef Fuctions_H
#define Fuctions_H
#include "Settings.h"

float getVoltage(uint8_t channel);

float getSignalVp(float voltage);

float getOwnBatteryVoltage();
float getBatteryVoltage();
float getSolarPannelVoltage();
bool hasClientConnected();
void testBoardVoltageElement(Stream &port);
void sendLoRaWan(float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec);
uint16_t calcCRC(const uint8_t *buf, uint8_t len);
void buildLoRaCommand(const uint8_t* data, size_t len, char* outBuffer, size_t outSize);
void goToSleep(unsigned long t0);

void sendJsonPost(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec);



#endif