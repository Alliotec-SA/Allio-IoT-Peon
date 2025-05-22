#include <ESP8266React.h>
#include <DeviceSettingsService.h>
#include <LightStateService.h>
#include "CycleAnalyzer.h"
#include <Adafruit_ADS1X15.h>
#include "Functions.h"
#include "Settings.h"





AsyncWebServer server(80);
ESP8266React esp8266React(&server);
DeviceSettingsService deviceSettingsService =
    DeviceSettingsService(&server, esp8266React.getFS(), esp8266React.getSecurityManager());
/*LightStateService lightStateService = LightStateService(&server,
                                                        esp8266React.getSecurityManager(),
                                                        esp8266React.getMqttClient(),
                                                        &lightMqttSettingsService);*/

Adafruit_ADS1115 ads;
CycleAnalyzer analyzer(PIN_SIGNAL, CHANNEL_SIGNAL, SIGNAL_TIMEOUT, ads);
unsigned long t0;


void setup() {
  // start serial and filesystem
  Serial.begin(SERIAL_BAUD_RATE);

  // start the framework and demo project
  esp8266React.begin();

  // load the initial light settings
  //lightStateService.begin();

  // start the device settings service
  deviceSettingsService.begin();

  // start the server
  server.begin();

  delay(2000);
  Serial.println(deviceSettingsService.getServer());
  Serial.println(deviceSettingsService.getPath());
  Serial.println(deviceSettingsService.getToken());
  Serial.println(deviceSettingsService.getDevEUI());
  ads.begin();
  analyzer.begin();
  t0 = millis()-(UPDATE_TIME-60000);
}

void loop() {
  // run the framework's loop function
  esp8266React.loop();
  analyzer.update();
  if(millis() - t0 > UPDATE_TIME){
      t0 = millis();
      analyzer.start();
      Serial.print("Start analyzing: ");
      //sendJsonPost(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), 12, 12/12, 7, getSignalVp(3.2),3900, 0);
  }
  if (analyzer.isReady()) {
    auto result = analyzer.getResult();
    
    if(!result.timeout) 
    {
      float v = getSignalVp(result.vMax);
      Serial.print("Periodo: "); Serial.println(result.periodMs);
      Serial.print("Vmin: "); Serial.println(result.vMin, 3);
      Serial.print("Vmax: "); Serial.println(result.vMax, 3);
      Serial.print("Vp: "); Serial.println(v);
      Serial.print("Baterry: "); Serial.println(getBatteryVoltage(), 4);
      Serial.print("Pannel: ");  Serial.println(getSolarPannelVoltage(), 4);
      sendJsonPost(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), getBatteryVoltage(), getBatteryVoltage()/12, getSolarPannelVoltage(), getSignalVp(result.vMax),result.periodMs, 0);
    }else{
      Serial.print("Timeout: "); Serial.println(result.periodMs);
    }
  }
}



