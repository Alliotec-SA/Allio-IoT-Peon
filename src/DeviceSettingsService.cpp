#include <DeviceSettingsService.h>

DeviceSettingsService::DeviceSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) :
    _httpEndpoint(DeviceSettings::read,
                  DeviceSettings::update,
                  this,
                  server,
                  DEVICE_SETTINGS_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _fsPersistence(DeviceSettings::read, DeviceSettings::update, this, fs, DEVICE_SETTINGS_FILE) {
}

bool DeviceSettingsService::isEnabled() {
  bool value;
  read([&](DeviceSettings& settings) {
    value = settings.enabled;
  });
  return value;
}

String DeviceSettingsService::getToken() {
  String value;
  read([&](DeviceSettings& settings) {
    value = settings.token;
  });
  return value;
}

String DeviceSettingsService::getDevEUI() {
  String value;
  read([&](DeviceSettings& settings) {
    value = settings.devEUI;
  });
  return value;
}

String DeviceSettingsService::getServer() {
  String value;
  read([&](DeviceSettings& settings) {
    value = settings.server;
  });
  return value;
}

String DeviceSettingsService::getPath() {
  String value;
  read([&](DeviceSettings& settings) {
    value = settings.path;
  });
  return value;
}

void DeviceSettingsService::begin() {
  _fsPersistence.readFromFS();
}
