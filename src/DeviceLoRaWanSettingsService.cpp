#include <DeviceLoRaWanSettingsService.h>

DeviceLoRaWanSettingsService::DeviceLoRaWanSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) :
    _httpEndpoint(DeviceLoRaWanSettings::read,
                  DeviceLoRaWanSettings::update,
                  this,
                  server,
                  DEVICE_LORAWAN_SETTINGS_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _fsPersistence(DeviceLoRaWanSettings::read, DeviceLoRaWanSettings::update, this, fs, DEVICE_LORAWAN_SETTINGS_FILE) {
}

bool DeviceLoRaWanSettingsService::shouldUseOtaa() {
  bool value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.use_otaa;
  });
  return value;
}

bool DeviceLoRaWanSettingsService::isEnabled() {
  bool value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.enabled;
  });
  return value;
}

String DeviceLoRaWanSettingsService::getDevEUI() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.devEUI;
  });
  return value;
}

String DeviceLoRaWanSettingsService::getAppEUI() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.appEUI;
  });
  return value;
}

String DeviceLoRaWanSettingsService::getAppKey() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.appKey;
  });
  return value;
}

String DeviceLoRaWanSettingsService::getNetKey() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.netKey;
  });
  return value;
}

String DeviceLoRaWanSettingsService::getDevAddress() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.devAddress;
  });
  return value;
}

String DeviceLoRaWanSettingsService::getAppsKey() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.appsKey;
  });
  return value;
}

String DeviceLoRaWanSettingsService::getNetsKey() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.netsKey;
  });
  return value;
}

void DeviceLoRaWanSettingsService::begin() {
  _fsPersistence.readFromFS();
}
