#ifndef DeviceStateService_h
#define DeviceStateService_h

#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <SettingValue.h>
#include "Settings.h"

#define DEVICE_STATE_FILE "/config/deviceState.json"
#define DEVICE_STATE_PATH "/rest/deviceState"

class DeviceState {
 public:
  LastResult lastResult;
  static void read(DeviceState& settings, JsonObject& root) {
    root["signal_voltage"] = String(settings.lastResult.signalVoltage);
    root["signa_period"] = String(settings.lastResult.signalPeriod);
    root["battery_voltage"] = String(settings.lastResult.batteryVoltage);
    root["battery_percent"] = String(settings.lastResult.batteryPercent);
    root["solar_voltage"] = String(settings.lastResult.solarVoltage);
    root["battery"] = String(settings.lastResult.battery);
    root["timeout"] = String(settings.lastResult.timeout);
    root["last_checked"] = String(settings.lastResult.lastChecked);
  }


  static StateUpdateResult update(JsonObject& root, DeviceState& settings) {
    return StateUpdateResult::CHANGED;
  }
};

class DeviceStateService : public StatefulService<DeviceState> {
 public:
  DeviceStateService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager);
  void updateLastValue(LastResult value);
  void begin();

 private:
  HttpEndpoint<DeviceState> _httpEndpoint;
  FSPersistence<DeviceState> _fsPersistence;
};

#endif  // end DeviceSettingsService_h
