#include "ReadSecuenceService.h"
#include <ArduinoJson.h>

#define DEVICE_STATE_FILE "/config/deviceState.json"

ReadSecuenceService readSecuenceService;

void ReadSecuenceService::begin(FS* fs) {
  _fs = fs;
  _value = 0;
  readFromFS();
}

uint8_t ReadSecuenceService::get() const {
  return _value;
}

uint8_t ReadSecuenceService::increment() {
  _value++;
  writeToFS();
  return _value;
}

void ReadSecuenceService::ensureConfigDir() {
  if (_fs && !_fs->exists("/config")) {
    _fs->mkdir("/config");
  }
}

void ReadSecuenceService::readFromFS() {
  if (!_fs) {
    return;
  }

  File file = _fs->open(READ_SECUENCE_FILE, "r");
  if (file) {
    DynamicJsonDocument doc(64);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error == DeserializationError::Ok) {
      _value = doc["read_secuence"] | _value;
      return;
    }
  }

  // Migrate legacy counter stored in deviceState.json
  File legacy = _fs->open(DEVICE_STATE_FILE, "r");
  if (!legacy) {
    return;
  }

  DynamicJsonDocument legacyDoc(512);
  DeserializationError legacyError = deserializeJson(legacyDoc, legacy);
  legacy.close();

  if (legacyError == DeserializationError::Ok && legacyDoc.containsKey("read_secuence")) {
    _value = legacyDoc["read_secuence"] | _value;
    writeToFS();
  }
}

bool ReadSecuenceService::writeToFS() {
  if (!_fs) {
    return false;
  }

  ensureConfigDir();

  DynamicJsonDocument doc(64);
  doc["read_secuence"] = _value;

  File file = _fs->open(READ_SECUENCE_FILE, "w");
  if (!file) {
    return false;
  }

  serializeJson(doc, file);
  file.close();
  return true;
}
