#ifndef DeviceLoRaWanSettingsService_h
#define DeviceLoRaWanSettingsService_h

#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <SettingValue.h>
#include "Settings.h"
#include "LoraWan.h"

#define DEVICE_LORAWAN_SETTINGS_FILE "/config/deviceLoraWanSettings.json"
#define DEVICE_LORAWAN_SETTINGS_PATH "/rest/deviceLoraWanSettings"



extern LoraWan lorawan; 

class DeviceLoRaWanSettings {
 public:
  String devEUI;
  String appEUI;
  String appKey;
  String netKey;
  String devAddress;
  String appsKey;
  String netsKey;
  bool use_otaa;
  bool enabled;
  bool isNewData = false;

  static void read(DeviceLoRaWanSettings& settings, JsonObject& root) {
    //char buffer[64];

    /*if (lorawan.getDevEUI(buffer, sizeof(buffer))) {
      settings.devEUI = String(buffer);
    }*/
    root["dev_eui"] = settings.devEUI;

    /*if (lorawan.getAppEUI(buffer, sizeof(buffer))) {
      settings.appEUI = String(buffer);
    }*/
    root["app_eui"] = settings.appEUI;

    /*if (lorawan.getAppKey(buffer, sizeof(buffer))) {
      settings.appKey = String(buffer);
    }*/
    root["app_key"] = settings.appKey;

    /*if (lorawan.getNetID(buffer, sizeof(buffer))) {
      settings.netKey = String(buffer);
    }*/
    root["net_key"] = settings.netKey;

    /*if (lorawan.getDevAddr(buffer, sizeof(buffer))) {
      settings.devAddress = String(buffer);
    }*/
    root["dev_address"] = settings.devAddress;

    /*if (lorawan.getAppSKey(buffer, sizeof(buffer))) {
      settings.appsKey = String(buffer);
    }*/
    root["apps_key"] = settings.appsKey;

    /*if (lorawan.getNwkSKey(buffer, sizeof(buffer))) {
      settings.netsKey = String(buffer);
    }*/
    root["nets_key"] = settings.netsKey;

    // Modo OTAA (leer AT+NJM=? y extraer valor)
    /*if (lorawan.getJoinMode(buffer, sizeof(buffer))) {
      settings.use_otaa = strstr(buffer, "=1") != nullptr;
    }*/
    root["use_otaa"] = settings.use_otaa;

    root["enabled"] = settings.enabled;
  }

  static StateUpdateResult update(JsonObject& root, DeviceLoRaWanSettings& settings) {
    settings.devEUI = root["dev_eui"] | SettingValue::format("");
    //lorawan.setDevEUI(settings.devEUI.c_str());

    settings.appEUI = root["app_eui"] | SettingValue::format("");
    //lorawan.setAppEUI(settings.appEUI.c_str());

    settings.appKey = root["app_key"] | SettingValue::format("");
    //lorawan.setAppKey(settings.appKey.c_str());

    settings.netKey = root["net_key"] | SettingValue::format("");
    //lorawan.setNetID(settings.netKey.c_str());

    settings.devAddress = root["dev_address"] | SettingValue::format("");
    //lorawan.setDevAddr(settings.devAddress.c_str());

    settings.appsKey = root["apps_key"] | SettingValue::format("");
    //lorawan.setAppSKey(settings.appsKey.c_str());

    settings.netsKey = root["nets_key"] | SettingValue::format("");
    //lorawan.setNwkSKey(settings.netsKey.c_str());

    settings.use_otaa = root["use_otaa"] | false;
    //lorawan.setJoinMode(settings.use_otaa);

    settings.enabled = root["enabled"] | true;
    settings.isNewData = true;

    return StateUpdateResult::CHANGED;
  }
};

class DeviceLoRaWanSettingsService : public StatefulService<DeviceLoRaWanSettings> {
 public:
  DeviceLoRaWanSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager);
  String getDevEUI();
  String getAppEUI();
  String getAppKey();
  String getNetKey();
  String getDevAddress();
  String getAppsKey();
  String getNetsKey();
  bool hasNewData();
  bool shouldUseOtaa();
  bool isEnabled();
  void begin();

 private:
  HttpEndpoint<DeviceLoRaWanSettings> _httpEndpoint;
  FSPersistence<DeviceLoRaWanSettings> _fsPersistence;
};

#endif  // end DeviceSettingsService_h
