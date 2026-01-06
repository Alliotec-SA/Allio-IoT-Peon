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
unsigned long tlastAnalyzerRun = 0;
bool hasGotValue = false; 
uint8_t readingTries = 0;
bool loraSent = false;
bool jsonSent = false;
unsigned long wifiWaitStart = 0;
bool waitingForWiFi = false;
boolean isElectrifierTurnedOn = false;
unsigned long runAnalyzerIntervalMs = UPDATE_TIME_IN_HOURS * 3600000;



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
  pinMode(PIN_TURN_ON_OFF_ELECTRIFIER, OUTPUT);
  digitalWrite(PIN_TURN_ON_OFF_ELECTRIFIER, LOW);
   


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
  deviceLoRaWanSettingsService.hasNewData(); // to avoid first glitch
  deviceStateService.begin();

  // Set base for ultra energy saving mode
  
  system_rtc_mem_read(RTC_ADDR, &rtcData, sizeof(rtcData));
  // Default to 0 if uninitialized
  if (rtcData.wakeup_cycle != 0 && rtcData.wakeup_cycle != 1) {
    rtcData.wakeup_cycle = 0;
  }

  if(rtcData.wakeup_cycle == 1){
    rtcData.wakeup_cycle = 0;
    system_rtc_mem_write(RTC_ADDR, &rtcData, sizeof(rtcData));
    SerialDebug.println("This is not the cycle");
    if(deviceStateService.isUltraEnergySavingMode())
      goToSleep(millis());
  }else{
    rtcData.wakeup_cycle = 1;
    system_rtc_mem_write(RTC_ADDR, &rtcData, sizeof(rtcData));
  }
  //end base ultra energy saving mode

  // Explicitly cast the function pointer to resolve overload ambiguity
  turnOnElectrifier(deviceStateService.isElectrifierTurnedOn(), &isElectrifierTurnedOn, &deviceStateService);

  // start the server
  server.begin();

  delay(200);


  ads.begin();
  ads.setDataRate(RATE_ADS1115_475SPS);
  Wire.setClock(100000);
  
  analyzer.begin();
  t0 = millis();
  tlastAnalyzerRun = t0;
  hasGotValue = false; 
  readingTries = 0;
  loraSent = false;
  jsonSent = false;
  lorawan.setLowPowerMode(false);
}

void loop() {
  esp8266React.loop();
  if(deviceLoRaWanSettingsService.hasNewData()){
    SerialDebug.println("Starting setup lora device");
    setupLoRaWan();
  }

  if(isElectrifierTurnedOn != deviceStateService.isElectrifierTurnedOn()){
    turnOnElectrifier(deviceStateService.isElectrifierTurnedOn(), &isElectrifierTurnedOn, &deviceStateService);
  } 

  analyzer.update();
  deviceStateService.updateIsRunningAnalyzingProcess(analyzer.isAnalyzerRunning());

  if(!analyzer.isAnalyzerRunning() && deviceStateService.startAnalyzingProcess()){
    SerialDebug.println("Start Analyzer from Device State Service");
    analyzer.start();
  }

  if(!deviceStateService.isUltraEnergySavingMode()){
    if(!analyzer.isAnalyzerRunning() && (millis() - tlastAnalyzerRun) >= runAnalyzerIntervalMs){
      analyzer.start();
      tlastAnalyzerRun = millis();
      SerialDebug.println("Start analyzing due to interval");
    }
  }
  
  if (analyzer.isReady()) {
    SerialDebug.println("Anlyzer is Ready ");
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
      SerialDebug.println("Timeout: "); SerialDebug.println(result.periodMs);
    }else{
      hasGotValue = true;
    }

    if(readingTries >= READING_TRIES){
      hasGotValue = true;
      SerialDebug.println("Max reading attempts reached.");
    }
      
      
    lastResult.signalPeriod = !result.timeout ? result.periodMs : 0;
    lastResult.signalVoltage = !result.timeout ? (int) getSignalVp(result.vMax) : 0;


    SerialDebug.print("Periodo: "); SerialDebug.println(lastResult.signalPeriod);
    SerialDebug.print("Vmin: "); SerialDebug.println(result.vMin, 3);
    SerialDebug.print("Vmax: "); SerialDebug.println(result.vMax, 3);
    SerialDebug.print("Vp: "); SerialDebug.println(lastResult.signalVoltage);
    SerialDebug.print("Baterry: "); SerialDebug.println(lastResult.batteryVoltage, 4);
    SerialDebug.print("Pannel: ");  SerialDebug.println(lastResult.solarVoltage, 4);
    SerialDebug.flush();

  }

  if(hasGotValue && !loraSent && deviceLoRaWanSettingsService.isEnabled()){
    SerialDebug.println("Sending By LoRa");
    sendLoRaWan(lastResult.batteryVoltage, lastResult.batteryPercent, lastResult.solarVoltage, lastResult.signalVoltage, lastResult.signalPeriod, lastResult.battery);
    loraSent = true;
  }

  if(hasGotValue && !jsonSent && deviceSettingsService.isEnabled() && WiFi.isConnected()){
    SerialDebug.println("Sending By Wifi");
    sendJsonPost(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), lastResult.batteryVoltage, lastResult.batteryPercent, lastResult.solarVoltage, lastResult.signalVoltage, lastResult.signalPeriod, lastResult.battery);
    jsonSent = true;
  }

  
  if(deviceStateService.isUltraEnergySavingMode() && !analyzer.isAnalyzerRunning() && !hasClientConnected() && millis()-t0 > MAX_TIME_TO_START_SETUP_IN_SECONDS*1000){
        goToSleep(t0);
  }

  //This condition should go to end, so make sure if ready condition can be evaluated
  if(!analyzer.isAnalyzerRunning() && readingTries < READING_TRIES && !hasGotValue){
      analyzer.start();
      SerialDebug.print("Start analyzing: ");
      SerialDebug.println(readingTries);
      SerialDebug.flush();
  }

}



