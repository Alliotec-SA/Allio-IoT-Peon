#include "Settings.h"
#include <ESP8266React.h>
#include <Adafruit_ADS1X15.h>
#include "Functions.h"
#include <DeviceSettingsService.h>
#include <DeviceStateService.h>
#include <DeviceLoRaWanSettingsService.h>
#include "CycleAnalyzer.h"
#include "LoraWan.h"
#include "SoftTimer.h"





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
SoftTimer timerRequestCommandsHTTP(5000);
SoftTimer timerLoraWanHeartbeat(LORAWAN_HEART_BEAT_INTERVAL_FOR_CLASS_C_IN_SECONDS * 1000);



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
  #ifdef FACTORY_DEVICE_MODEL
    SerialDebug.print("Device Model: ");
    SerialDebug.println(FACTORY_DEVICE_MODEL);
  #endif
  #ifdef FACTORY_DEVICE_HARDWARE_VERSION
    SerialDebug.print("Device Hardware Version: ");
    SerialDebug.println(FACTORY_DEVICE_HARDWARE_VERSION);
  #endif
  #ifdef FACTORY_DEVICE_FIRMWARE_VERSION
    SerialDebug.print("Device Firmware Version: ");
    SerialDebug.println(FACTORY_DEVICE_FIRMWARE_VERSION);
  #endif
  #ifdef FACTORY_DEVICE_MANUFACTURER
    SerialDebug.print("Device Manufacturer: ");
    SerialDebug.println(FACTORY_DEVICE_MANUFACTURER);
  #endif

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
  lorawan.enableATMode();
  lorawan.setLowPowerMode(false);
  timerRequestCommandsHTTP.start();
  timerLoraWanHeartbeat.start();
  timerLoraWanHeartbeat.setInterval(deviceLoRaWanSettingsService.getClassMode() == "C" ? LORAWAN_HEART_BEAT_INTERVAL_FOR_CLASS_C_IN_SECONDS * 1000 : LORAWAN_HEART_BEAT_INTERVAL_FOR_CLASS_A_IN_SECONDS * 1000);
  lorawan.sendHeartbeat(); // First heartbeat
}

void loop() {
  esp8266React.loop();
  if(deviceLoRaWanSettingsService.hasNewData()){
    SerialDebug.println("Starting setup lora device");
    timerLoraWanHeartbeat.setInterval(deviceLoRaWanSettingsService.getClassMode() == "C" ? LORAWAN_HEART_BEAT_INTERVAL_FOR_CLASS_C_IN_SECONDS * 1000 : LORAWAN_HEART_BEAT_INTERVAL_FOR_CLASS_A_IN_SECONDS * 1000);
    setupLoRaWan();
  }

  if(isElectrifierTurnedOn != deviceStateService.isElectrifierTurnedOn()){
    turnOnElectrifier(deviceStateService.isElectrifierTurnedOn(), &isElectrifierTurnedOn, &deviceStateService);
  } 

  analyzer.update();
  deviceStateService.updateIsRunningAnalyzingProcess(analyzer.isAnalyzerRunning());
  if(!analyzer.isAnalyzerRunning() && !deviceStateService.isUltraEnergySavingMode() && deviceLoRaWanSettingsService.isEnabled() && timerLoraWanHeartbeat.elapsed()){
    SerialDebug.println("Sending LoRaWAN Heartbeat");
    lorawan.sendHeartbeat();
    timerLoraWanHeartbeat.reset();
  }

  if(!analyzer.isAnalyzerRunning() && deviceSettingsService.isEnabled() && timerRequestCommandsHTTP.elapsed()){
    SerialDebug.println("Requesting commands over HTTP");
    requestCommandsOverHTTP();
    timerRequestCommandsHTTP.reset();
  }

  // Check if we should start analyzing process from manual or remote activation
  if(!analyzer.isAnalyzerRunning() && deviceStateService.startAnalyzingProcess()){
    SerialDebug.println("Start Analyzer from Device State Service");
    analyzer.start();
  }

  //Check if we should start analyzing process from interval for non ultra energy saving mode or in full wakeup for ultra energy saving mode
  if(!deviceStateService.isUltraEnergySavingMode()){
    if(!analyzer.isAnalyzerRunning() && (millis() - tlastAnalyzerRun) >= runAnalyzerIntervalMs){
      analyzer.start();
      tlastAnalyzerRun = millis();
      SerialDebug.println("Start analyzing due to interval");
    }
  }else if(!analyzer.isAnalyzerRunning() && !analyzer.isReady() && readingTries < READING_TRIES && !hasGotValue){
    SerialDebug.println("Start analyzing in ultra energy saving mode due to max time without client connection");
    analyzer.start();
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
    

    #ifndef DEBUG_SENT_DATA_EVEN_IF_TIMEOUT
      if(result.timeout){
        SerialDebug.println("Timeout: "); SerialDebug.println(result.periodMs);
      }else{
        hasGotValue = true;
      }
    #endif
    #ifdef DEBUG_SENT_DATA_EVEN_IF_TIMEOUT
      result.timeout = false; // For debug purposes, ignore timeout and send data anyway
      result.ready = true;
      result.periodMs = 3000;
      result.vMin = 0.01;
      result.vMax = 5.0;
      lastResult.ready = true;
      lastResult.batteryVoltage = 12.5;
      lastResult.batteryPercent = 100;
      lastResult.solarVoltage = 15.5;
      lastResult.battery = 12.5;
      hasGotValue = true;
    #endif

    if(readingTries >= READING_TRIES){
      hasGotValue = true;
      SerialDebug.println("Max reading attempts reached.");
    }

    if(result.timeout){
      SerialDebug.println("Signal timeout detected.");
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

  // Send data by LoRaWAN if we got value and LoRaWAN is enabled, if not enabled mark as sent to reset params in next cycle
  if(hasGotValue && !loraSent && deviceLoRaWanSettingsService.isEnabled()){
    SerialDebug.println("Sending By LoRa");
    sendLoRaWan(lastResult.batteryVoltage, lastResult.batteryPercent, lastResult.solarVoltage, lastResult.signalVoltage, lastResult.signalPeriod, lastResult.battery);
    loraSent = true;
  }else if(hasGotValue && !deviceLoRaWanSettingsService.isEnabled()){
    loraSent = true; //mark to reset params
  }

  // Send data by HTTP if we got value and HTTP is enabled and WiFi is connected, if HTTP is not enabled mark as sent to reset params in next cycle, if WiFi is not connected start waiting for WiFi and send when it gets connected
  if(hasGotValue && !jsonSent && deviceSettingsService.isEnabled() && WiFi.isConnected()){
    SerialDebug.println("Sending By Wifi");
    sendDeviceDataByHttp(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), lastResult.batteryVoltage, lastResult.batteryPercent, lastResult.solarVoltage, lastResult.signalVoltage, lastResult.signalPeriod, lastResult.battery, isElectrifierTurnedOn);
    jsonSent = true;
  }else if(hasGotValue && !deviceSettingsService.isEnabled()){
    jsonSent = true; //mark to reset params
  }

  // If we got value and sent by LoRaWAN and HTTP, we can reset state for next cycle 
  if(hasGotValue && loraSent && jsonSent && !deviceStateService.isUltraEnergySavingMode()){
      jsonSent = false;
      loraSent = false;
      readingTries = 0; // reset tries for next cycle
      hasGotValue = false; // reset value for next cycle
      SerialDebug.println("Cycle completed, reset state for next cycle");
  }

  
  if(deviceStateService.isUltraEnergySavingMode() && !analyzer.isAnalyzerRunning() && !hasClientConnected() && millis()-t0 > MAX_TIME_TO_START_SETUP_IN_SECONDS*1000){
        goToSleep(t0);
  }

 
  //In serial I received thes message format (+EVT:RX_C:-64:5:UNICAST:4:b076e8198c6454f77c56) that is
  //RX_C | RSSI | SNR | TYPE | FPORT | PAYLOAD_HEX   I need to detect when this frame is detected I get params values, but Once Serial available try to read and keep wainting until  end of frame comes from rak3172 or timeout and evaluate
  if(Serial.available()){
    bool wasRunningAnalizer = analyzer.isAnalyzerRunning();
    if(wasRunningAnalizer){
      analyzer.cancel();
      SerialDebug.println("Analyzer stopped to process LoRaWAN command");
    }

    String line = Serial.readStringUntil('\n');
    //avoid any out exact chat at end
    line.trim();

    SerialDebug.print("LoRaWAN Serial Received: ");
    SerialDebug.println(line);
    if(line.startsWith("+EVT:RX_C:")){
      processReceivedLoRaWanCommand(line);
    }

    if(wasRunningAnalizer){
      analyzer.start();
      SerialDebug.println("Analyzer restarted after processing LoRaWAN command");
    }
  }
}