#include "Settings.h"
#include <ESP8266React.h>
#include <Adafruit_ADS1X15.h>
#include "Functions.h"
#include <DeviceSettingsService.h>
#include <DeviceStateService.h>
#include <DeviceLoRaWanSettingsService.h>
#include "CycleAnalyzer.h"
#include "LoraWan.h"





AsyncWebServer server(80);
ESP8266React esp8266React(&server);
DeviceSettingsService deviceSettingsService =
    DeviceSettingsService(&server, esp8266React.getFS(), esp8266React.getSecurityManager());
DeviceStateService deviceStateService =
    DeviceStateService(&server, esp8266React.getFS(), esp8266React.getSecurityManager());
DeviceLoRaWanSettingsService deviceLoRaWanSettingsService = DeviceLoRaWanSettingsService(&server, esp8266React.getFS(), esp8266React.getSecurityManager());

Adafruit_ADS1115 ads;
CycleAnalyzer analyzer(PIN_SIGNAL, CHANNEL_SIGNAL, SIGNAL_TIMEOUT, ads);
LastResult lastResult;

unsigned long t0 = 0;
bool hasGotValue = false; 
uint8_t readingTries = 0;


struct RTCData {
  uint8_t wakeup_cycle;  // 0 or 1
};

RTCData rtcData;
LoraWan lorawan(Serial);

void setup() {
  //Wake Up Settings
  pinMode(PIN_DRST, OUTPUT);
  digitalWrite(PIN_DRST, LOW);
  pinMode(PIN_WKP, WAKEUP_PULLUP);

  system_rtc_mem_read(RTC_ADDR, &rtcData, sizeof(rtcData));
  // Default to 0 if uninitialized
  if (rtcData.wakeup_cycle != 0 && rtcData.wakeup_cycle != 1) {
    rtcData.wakeup_cycle = 0;
  }

  if(rtcData.wakeup_cycle == 1){
    rtcData.wakeup_cycle = 0;
    system_rtc_mem_write(RTC_ADDR, &rtcData, sizeof(rtcData));
    goToSleep(millis());
  }else{
    rtcData.wakeup_cycle = 1;
    system_rtc_mem_write(RTC_ADDR, &rtcData, sizeof(rtcData));
  }

  // start serial and filesystem
  Serial.begin(SERIAL_BAUD_RATE);
  SerialDebug.begin(SERIAL_BAUD_RATE);
  
  SerialDebug.println("Working wakeup");

  // start the framework and demo project
  esp8266React.begin();
  

  // load the initial light settings
  //lightStateService.begin();

  // start the device settings service
  deviceSettingsService.begin();
  deviceLoRaWanSettingsService.begin();
  deviceStateService.begin();

  // start the server
  server.begin();


  ads.begin();
  analyzer.begin();
  t0 = millis();

}

void loop() {
  esp8266React.loop();
  analyzer.update();

  if(!analyzer.isAnalyzerRunning() && readingTries < READING_TRIES && !hasGotValue){
      analyzer.start();
      SerialDebug.print("Start analyzing: ");
  }

  if (analyzer.isReady()) {
    readingTries++;
    auto result = analyzer.getResult();
    lastResult.batteryVoltage = getBatteryVoltage();
    lastResult.batteryPercent = ((int)lastResult.batteryVoltage/12)*100;
    lastResult.solarVoltage = getSolarPannelVoltage();
    lastResult.battery = getOwnBatteryVoltage();
    lastResult.ready = result.ready;
    lastResult.timeout = result.timeout;
    lastResult.lastChecked = millis();
    deviceStateService.updateLastValue(lastResult);
    

    if(result.timeout){
      SerialDebug.print("Timeout: "); SerialDebug.println(result.periodMs);
    }else{
      hasGotValue = true;
    }
      
    lastResult.signalPeriod = !result.timeout ? result.periodMs : 0;
    lastResult.signalVoltage = (int) getSignalVp(result.vMax);


    SerialDebug.print("Periodo: "); SerialDebug.println(lastResult.signalPeriod);
    SerialDebug.print("Vmin: "); SerialDebug.println(result.vMin, 3);
    SerialDebug.print("Vmax: "); SerialDebug.println(result.vMax, 3);
    SerialDebug.print("Vp: "); SerialDebug.println(lastResult.signalVoltage);
    SerialDebug.print("Baterry: "); SerialDebug.println(lastResult.batteryVoltage, 4);
    SerialDebug.print("Pannel: ");  SerialDebug.println(lastResult.solarVoltage, 4);

    if(hasGotValue || readingTries > READING_TRIES){
      if(deviceSettingsService.isEnabled() && WiFi.isConnected()){
        sendJsonPost(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), lastResult.batteryVoltage, lastResult.batteryPercent, lastResult.solarVoltage, lastResult.signalVoltage, lastResult.signalPeriod, lastResult.battery);
      }

      if(deviceLoRaWanSettingsService.isEnabled()){
        sendLoRaWan(lastResult.batteryVoltage, lastResult.batteryPercent, lastResult.solarVoltage, lastResult.signalVoltage, lastResult.signalPeriod, lastResult.battery);
      }
    }
  }

  if(analyzer.isReady() && !hasClientConnected && millis()-t0 > MAX_TIME_TO_START_SETUP_IN_SECONDS*1000){
        goToSleep(t0);
  }
}



