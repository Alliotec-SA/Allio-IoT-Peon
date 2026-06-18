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
  bool use_otaa = LORAWAN_DEFAULT_USE_OTAA;
  bool enabled = true;
  bool isNewData = false;
  uint8_t band = LORAWAN_DEFAULT_BAND;
  uint8_t subBand = LORAWAN_DEFAULT_SUB_BAND;
  uint8_t dataRate = LORAWAN_DEFAULT_DATA_RATE;
  uint8_t rx2Dr = LORAWAN_DEFAULT_RX2_DR;
  uint32_t rx2FreqHz = LORAWAN_DEFAULT_RX2_FREQ_HZ;
  bool adr = LORAWAN_DEFAULT_ADR;
  bool confirmMode = LORAWAN_DEFAULT_CONFIRM_MODE;
  String classMode = LORAWAN_DEFAULT_CLASS_MODE;

  static void read(DeviceLoRaWanSettings& settings, JsonObject& root) {
    root["dev_eui"] = settings.devEUI;
    root["app_eui"] = settings.appEUI;
    root["app_key"] = settings.appKey;
    root["net_key"] = settings.netKey;
    root["dev_address"] = settings.devAddress;
    root["apps_key"] = settings.appsKey;
    root["nets_key"] = settings.netsKey;
    root["use_otaa"] = settings.use_otaa;
    root["class_mode"] = settings.classMode;
    root["enabled"] = settings.enabled;
    root["band"] = settings.band;
    root["sub_band"] = settings.subBand;
    root["data_rate"] = settings.dataRate;
    root["rx2_dr"] = settings.rx2Dr;
    root["rx2_freq_hz"] = settings.rx2FreqHz;
    root["adr"] = settings.adr;
    root["confirm_mode"] = settings.confirmMode;
  }

  static StateUpdateResult update(JsonObject& root, DeviceLoRaWanSettings& settings) {
    settings.devEUI = root["dev_eui"] | SettingValue::format("");
    settings.appEUI = root["app_eui"] | SettingValue::format("");
    settings.appKey = root["app_key"] | SettingValue::format("");
    settings.netKey = root["net_key"] | SettingValue::format("");
    settings.devAddress = root["dev_address"] | SettingValue::format("");
    settings.appsKey = root["apps_key"] | SettingValue::format("");
    settings.netsKey = root["nets_key"] | SettingValue::format("");
    settings.use_otaa = root["use_otaa"] | LORAWAN_DEFAULT_USE_OTAA;
    settings.enabled = root["enabled"] | true;
    settings.classMode = root["class_mode"] | SettingValue::format(LORAWAN_DEFAULT_CLASS_MODE);
    settings.band = root["band"] | LORAWAN_DEFAULT_BAND;
    settings.subBand = root["sub_band"] | LORAWAN_DEFAULT_SUB_BAND;
    settings.dataRate = root["data_rate"] | LORAWAN_DEFAULT_DATA_RATE;
    settings.rx2Dr = root["rx2_dr"] | LORAWAN_DEFAULT_RX2_DR;
    settings.rx2FreqHz = root["rx2_freq_hz"] | LORAWAN_DEFAULT_RX2_FREQ_HZ;
    settings.adr = root["adr"] | LORAWAN_DEFAULT_ADR;
    settings.confirmMode = root["confirm_mode"] | LORAWAN_DEFAULT_CONFIRM_MODE;
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
  String getClassMode();
  uint8_t getBand();
  uint8_t getSubBand();
  uint8_t getDataRate();
  uint8_t getRx2Dr();
  uint32_t getRx2FreqHz();
  bool getAdr();
  bool getConfirmMode();
  bool hasNewData();
  bool shouldUseOtaa();
  bool isEnabled();
  void begin();

 private:
  HttpEndpoint<DeviceLoRaWanSettings> _httpEndpoint;
  FSPersistence<DeviceLoRaWanSettings> _fsPersistence;
};

#endif  // end DeviceSettingsService_h
