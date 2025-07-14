#ifndef DeviceLoRaWanSettingsService_h
#define DeviceLoRaWanSettingsService_h

#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <SettingValue.h>

#define DEVICE_LORAWAN_SETTINGS_FILE "/config/deviceLoraWanSettings.json"
#define DEVICE_LORAWAN_SETTINGS_PATH "/rest/deviceLoraWanSettings"

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

  static void read(DeviceLoRaWanSettings& settings, JsonObject& root) {
    root["dev_eui"] = settings.devEUI;
    root["app_eui"] = settings.appEUI;
    root["app_key"] = settings.appKey;
    root["net_key"] = settings.netKey;
    root["dev_address"] = settings.devAddress;
    root["apps_key"] = settings.appsKey;
    root["nets_key"] = settings.netsKey;
    root["use_otaa"] = settings.use_otaa;
  }

  static StateUpdateResult update(JsonObject& root, DeviceLoRaWanSettings& settings) {
    settings.devEUI = root["dev_eui"] | SettingValue::format("");
    settings.appEUI = root["app_eui"] | SettingValue::format("");
    settings.appKey = root["app_key"] | SettingValue::format("");
    settings.netKey = root["net_key"] | SettingValue::format("");
    settings.devAddress = root["dev_address"] | SettingValue::format("") ;
    settings.appsKey = root["apps_key"]  | SettingValue::format("") ;
    settings.netsKey = root["nets_key"]  | SettingValue::format("");
    settings.use_otaa = root["use_otaa"] | false;
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
  bool shouldUseOtaa();
  void begin();

 private:
  HttpEndpoint<DeviceLoRaWanSettings> _httpEndpoint;
  FSPersistence<DeviceLoRaWanSettings> _fsPersistence;
};

#endif  // end DeviceSettingsService_h
