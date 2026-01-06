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
  boolean isRunningAnalyzingProcess = false;
  boolean startAnalyzingProcess = false;
  boolean isTurnedOn = true;
  boolean isUltraEnergySavingMode = false;

  static void read(DeviceState& settings, JsonObject& root) {
    root["signal_voltage"] = String(settings.lastResult.signalVoltage);
    root["signa_period"] = String(settings.lastResult.signalPeriod);
    root["battery_voltage"] = String(settings.lastResult.batteryVoltage);
    root["battery_percent"] = String(settings.lastResult.batteryPercent);
    root["solar_voltage"] = String(settings.lastResult.solarVoltage);
    root["battery"] = String(settings.lastResult.battery);
    root["timeout"] = String(settings.lastResult.timeout);
    root["last_checked"] = String(settings.lastResult.lastChecked);
    root["is_running_analyzing_process"] = settings.isRunningAnalyzingProcess;
    root["is_turned_on"] = settings.isTurnedOn;
    root["is_ultra_energy_saving_mode"] = settings.isUltraEnergySavingMode;
  }


  static StateUpdateResult update(JsonObject& root, DeviceState& settings) {
    settings.startAnalyzingProcess = root["start_analyzing_process"] | settings.startAnalyzingProcess;
    if(settings.startAnalyzingProcess) {
        settings.isRunningAnalyzingProcess = true;
    }
    settings.isTurnedOn = root["is_turned_on"] | settings.isTurnedOn;
    settings.isUltraEnergySavingMode = root["is_ultra_energy_saving_mode"] | settings.isUltraEnergySavingMode;
    return StateUpdateResult::CHANGED;
  }
};

class DeviceStateService : public StatefulService<DeviceState> {
 public:
  DeviceStateService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager);
  void updateLastValue(LastResult value);
  void updateIsRunningAnalyzingProcess(boolean value);
  void updateElectrifierState(boolean value);
  boolean isElectrifierTurnedOn();
  boolean startAnalyzingProcess();
  boolean isUltraEnergySavingMode();
  void begin();

 private:
  HttpEndpoint<DeviceState> _httpEndpoint;
  FSPersistence<DeviceState> _fsPersistence;
};

#endif  // end DeviceSettingsService_h
