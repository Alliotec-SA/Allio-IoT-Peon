#include <DeviceStateService.h>

DeviceStateService::DeviceStateService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) :
    _httpEndpoint(DeviceState::read,
                  DeviceState::update,
                  this,
                  server,
                  DEVICE_STATE_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _fsPersistence(DeviceState::read, DeviceState::update, this, fs, DEVICE_STATE_FILE) {
}

void DeviceStateService::updateLastValue(LastResult value){
  read([&](DeviceState& settings) {
    settings.lastResult = value;
  });
}

void DeviceStateService::updateIsRunningAnalyzingProcess(boolean value){
  read([&](DeviceState& settings) {
    settings.isRunningAnalyzingProcess = value;
  });
} 

void DeviceStateService::updateElectrifierState(boolean value){
  read([&](DeviceState& settings) {
    settings.isTurnedOn = value;
  });
}

boolean DeviceStateService::isElectrifierTurnedOn(){
  boolean value;
  read([&](DeviceState& settings) {
    value = settings.isTurnedOn;
  });
  return value;
}

boolean DeviceStateService::startAnalyzingProcess() {
  boolean value;
  read([&](DeviceState& settings) {
    value = settings.startAnalyzingProcess;
    settings.startAnalyzingProcess = false;
  });
  return value;
}

boolean DeviceStateService::isUltraEnergySavingMode(){
  boolean value;
  read([&](DeviceState& settings) {
    value = settings.isUltraEnergySavingMode;
  });
  return value;
}

boolean DeviceStateService::consumeElectrifierAckPending() {
  boolean pending = false;
  read([&](DeviceState& settings) {
    pending = settings.electrifierAckPending;
    settings.electrifierAckPending = false;
  });
  return pending;
}

void DeviceStateService::begin() {
  _fsPersistence.readFromFS();
}
