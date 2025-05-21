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

void DeviceSettingsService::begin() {
  _fsPersistence.readFromFS();
}
