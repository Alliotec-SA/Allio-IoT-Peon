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

bool DeviceLoRaWanSettingsService::hasNewData() {
  bool value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.isNewData;
    settings.isNewData = false;
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

String DeviceLoRaWanSettingsService::getClassMode() {
  String value;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.classMode;
  });
  return value;
}

uint8_t DeviceLoRaWanSettingsService::getBand() {
  uint8_t value = LORAWAN_DEFAULT_BAND;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.band;
  });
  return value;
}

uint8_t DeviceLoRaWanSettingsService::getSubBand() {
  uint8_t value = LORAWAN_DEFAULT_SUB_BAND;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.subBand;
  });
  return value;
}

uint8_t DeviceLoRaWanSettingsService::getDataRate() {
  uint8_t value = LORAWAN_DEFAULT_DATA_RATE;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.dataRate;
  });
  return value;
}

uint8_t DeviceLoRaWanSettingsService::getRx2Dr() {
  uint8_t value = LORAWAN_DEFAULT_RX2_DR;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.rx2Dr;
  });
  return value;
}

uint32_t DeviceLoRaWanSettingsService::getRx2FreqHz() {
  uint32_t value = LORAWAN_DEFAULT_RX2_FREQ_HZ;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.rx2FreqHz;
  });
  return value;
}

bool DeviceLoRaWanSettingsService::getAdr() {
  bool value = LORAWAN_DEFAULT_ADR;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.adr;
  });
  return value;
}

bool DeviceLoRaWanSettingsService::getConfirmMode() {
  bool value = LORAWAN_DEFAULT_CONFIRM_MODE;
  read([&](DeviceLoRaWanSettings& settings) {
    value = settings.confirmMode;
  });
  return value;
}

void DeviceLoRaWanSettingsService::begin() {
  _fsPersistence.readFromFS();
}
