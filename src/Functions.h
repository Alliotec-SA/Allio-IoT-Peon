#ifndef Fuctions_H
#define Fuctions_H
#include "Settings.h"
#include "LoraWan.h"
#include <ESP8266React.h>
#include <DeviceLoRaWanSettingsService.h>
#include <DeviceStateService.h>
#include <DeviceSettingsService.h>


extern LoraWan lorawan; 
extern ESP8266React esp8266React;
extern DeviceLoRaWanSettingsService deviceLoRaWanSettingsService;
extern DeviceSettingsService deviceSettingsService;

extern DeviceStateService deviceStateService;
extern boolean isElectrifierTurnedOn;

float getVoltage(uint8_t channel);

float getSignalVp(float voltage);

float getOwnBatteryVoltage();
float getBatteryVoltage();
float getSolarPannelVoltage();
bool hasClientConnected();
void testBoardVoltageElement(Stream &port);
void sendLoRaWanCommandACK(uint8_t command, boolean executionCommandDone);
void ackCommandOnActiveChannels(const String &httpAction, uint8_t loraCmd, boolean executionCommandDone, const String &httpResponse);
void sendLoRaWan(float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec);
uint16_t calcCRC(const uint8_t *buf, uint8_t len);
void bytesToHexString(const uint8_t* data, size_t len, char* outHex, size_t outLen);
void goToSleep(unsigned long t0);
void resetWifiSettings();
void setupLoRaWan();
void applyLoRaWanFlashConfig();
bool syncLoRaWanFromFlash();
struct ElectrifierAckContext {
  const char *httpAction;
  uint8_t loraCmd;
  boolean success;
  const char *response;
};

void turnOnElectrifier(boolean state, boolean *isTurnedOn, DeviceStateService *deviceStateService, const ElectrifierAckContext *ack = nullptr);

void sendHttpPostJson(String tag, String host, String path, String token, String devEUI, String json);
void sendDeviceDataByHttp(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec, uint8_t readSecuence, boolean isElectrifierTurnedOn);
void ackCommandPost(String host, String path, String token, String devEUI, String command, boolean executionCommandDone, String response);
bool getCommandsByHTTP(String host, String path, String token, String devEUI, void (*callback)(const String&));
void requestCommandsOverHTTP();
void callbackForHttpCommands(const String& command);
void LoRaWanExecuteDownloadedCommands(LoRaWanDownlinkContext* ctx);
bool LoRaWanValidateAllDownloadedCommands(const LoRaWanDownlinkContext* ctx);
bool LoRaWanParseDownlink(const String& payloadHex, LoRaWanDownlinkContext* ctx);
uint8_t hexToU8(const String& hex);
void processReceivedLoRaWanCommand(const String &line);


#endif