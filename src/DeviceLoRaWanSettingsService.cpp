#include <DeviceLoRaWanSettingsService.h>

DeviceSettingsService::DeviceSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) :
    _httpEndpoint(DeviceLoRaWanSettings::read,
                  DeviceLoRaWanSettings::update,
                  this,
                  server,
                  DEVICE_SETTINGS_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _fsPersistence(DeviceLoRaWanSettings::read, DeviceLoRaWanSettings::update, this, fs, DEVICE_SETTINGS_FILE) {
}

bool DeviceSettingsService::shouldUseOtaa() {
  bool value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.use_otaa;
  });
  return value;
}

String DeviceSettingsService::getDevEUI() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.devEUI;
  });
  return value;
}

String DeviceSettingsService::getAppEUI() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.appEUI;
  });
  return value;
}

String DeviceSettingsService::getAppKey() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.appKey;
  });
  return value;
}

String DeviceSettingsService::getNetKey() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.netKey;
  });
  return value;
}

String DeviceSettingsService::getDevAddress() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.devAddress;
  });
  return value;
}

String DeviceSettingsService::getAppsKey() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.appsKey;
  });
  return value;
}

String DeviceSettingsService::getNetsKey() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.netsKey;
  });
  return value;
}

void DeviceSettingsService::begin() {
  _fsPersistence.readFromFS();
}
