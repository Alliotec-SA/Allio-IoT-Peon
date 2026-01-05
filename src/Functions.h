#ifndef Fuctions_H
#define Fuctions_H
#include "Settings.h"
#include "LoraWan.h"
#include <ESP8266React.h>
#include <DeviceLoRaWanSettingsService.h>


extern LoraWan lorawan; 
extern ESP8266React esp8266React;
extern DeviceLoRaWanSettingsService deviceLoRaWanSettingsService;

float getVoltage(uint8_t channel);

float getSignalVp(float voltage);

float getOwnBatteryVoltage();
float getBatteryVoltage();
float getSolarPannelVoltage();
bool hasClientConnected();
void testBoardVoltageElement(Stream &port);
void sendLoRaWan(float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec);
uint16_t calcCRC(const uint8_t *buf, uint8_t len);
void bytesToHexString(const uint8_t* data, size_t len, char* outHex, size_t outLen);
void goToSleep(unsigned long t0);
void resetWifiSettings();
void setupLoRaWan();
void turnOffElectrifier();
void turnOnElectrifier();

void sendJsonPost(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec);



#endif