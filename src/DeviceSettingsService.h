#ifndef DeviceSettingsService_h
#define DeviceSettingsService_h

#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <SettingValue.h>

#define DEVICE_SETTINGS_FILE "/config/deviceSettings.json"
#define DEVICE_SETTINGS_PATH "/rest/deviceSettings"

class DeviceSettings {
 public:
  String devEUI;
  String token;
  String server;
  String path;

  static void read(DeviceSettings& settings, JsonObject& root) {
    root["dev_eui"] = settings.devEUI;
    root["token"] = settings.token;
    root["server"] = settings.server;
    root["path"] = settings.path;
  }

  static StateUpdateResult update(JsonObject& root, DeviceSettings& settings) {
    settings.devEUI = root["dev_eui"] | SettingValue::format("");
    settings.token = root["token"] | SettingValue::format("");
    settings.server = root["server"] | SettingValue::format("apismartweather.alliotec.com");
    settings.path = root["path"] | SettingValue::format("/api/v2/peon");
    return StateUpdateResult::CHANGED;
  }
};

class DeviceSettingsService : public StatefulService<DeviceSettings> {
 public:
  DeviceSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager);
  String getToken();
  String getDevEUI();
  String getServer();
  String getPath();
  void begin();

 private:
  HttpEndpoint<DeviceSettings> _httpEndpoint;
  FSPersistence<DeviceSettings> _fsPersistence;
};

#endif  // end DeviceSettingsService_h
