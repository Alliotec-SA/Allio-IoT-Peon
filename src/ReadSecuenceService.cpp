#include "ReadSecuenceService.h"
#include "Settings.h"
#include <ArduinoJson.h>

#define DEVICE_STATE_FILE "/config/deviceState.json"
#define READ_SEQ_CHECKSUM_SALT 0xA5

ReadSecuenceService readSecuenceService;

uint8_t ReadSecuenceService::checksum(uint8_t value, uint32_t generation, uint8_t slot) {
  uint8_t crc = READ_SEQ_CHECKSUM_SALT;
  crc ^= value;
  crc ^= (uint8_t)(generation & 0xFF);
  crc ^= (uint8_t)((generation >> 8) & 0xFF);
  crc ^= (uint8_t)((generation >> 16) & 0xFF);
  crc ^= (uint8_t)((generation >> 24) & 0xFF);
  crc ^= slot;
  return crc;
}

void ReadSecuenceService::begin(FS* fs) {
  _fs = fs;
  _value = 0;
  _activeSlot = 0;
  _generation = 0;
  readFromFS();
}

uint8_t ReadSecuenceService::get() const {
  return _value;
}

uint8_t ReadSecuenceService::increment() {
  const uint8_t nextValue = _value + 1;
  const uint32_t nextGeneration = _generation + 1;

  for (uint8_t attempt = 0; attempt < READ_SEQ_SLOT_COUNT; attempt++) {
    const uint8_t slot = (uint8_t)((_activeSlot + 1 + attempt) % READ_SEQ_SLOT_COUNT);
    if (!writeSlot(slot, nextValue, nextGeneration)) {
      continue;
    }
    _activeSlot = slot;
    _value = nextValue;
    _generation = nextGeneration;
    return _value;
  }

  SerialDebug.println("ReadSecuence: all slot writes failed, keeping RAM value only");
  _value = nextValue;
  _generation = nextGeneration;
  return _value;
}

void ReadSecuenceService::ensureDirs() {
  if (!_fs) {
    return;
  }
  if (!_fs->exists("/config")) {
    _fs->mkdir("/config");
  }
  if (!_fs->exists(READ_SEQ_DIR)) {
    _fs->mkdir(READ_SEQ_DIR);
  }
}

bool ReadSecuenceService::writeSlot(uint8_t slot, uint8_t value, uint32_t generation) {
  if (!_fs || slot >= READ_SEQ_SLOT_COUNT) {
    return false;
  }

  ensureDirs();

  char path[24];
  snprintf(path, sizeof(path), READ_SEQ_DIR "/s%u", slot);

  const uint8_t record[READ_SEQ_RECORD_SIZE] = {
      value,
      (uint8_t)(generation & 0xFF),
      (uint8_t)((generation >> 8) & 0xFF),
      (uint8_t)((generation >> 16) & 0xFF),
      (uint8_t)((generation >> 24) & 0xFF),
      slot,
      checksum(value, generation, slot),
      READ_SEQ_RECORD_VERSION,
      0x00};

  File file = _fs->open(path, "w");
  if (!file) {
    return false;
  }

  const size_t written = file.write(record, sizeof(record));
  file.close();
  return written == sizeof(record);
}

bool ReadSecuenceService::readSlot(uint8_t slot, uint8_t& value, uint32_t& generation) const {
  if (!_fs || slot >= READ_SEQ_SLOT_COUNT) {
    return false;
  }

  char path[24];
  snprintf(path, sizeof(path), READ_SEQ_DIR "/s%u", slot);

  File file = _fs->open(path, "r");
  if (!file) {
    return false;
  }

  uint8_t record[READ_SEQ_RECORD_SIZE] = {0};
  const size_t bytesRead = file.read(record, sizeof(record));
  file.close();

  if (bytesRead != sizeof(record) || record[7] != READ_SEQ_RECORD_VERSION) {
    return false;
  }

  const uint32_t storedGeneration =
      (uint32_t)record[1] |
      ((uint32_t)record[2] << 8) |
      ((uint32_t)record[3] << 16) |
      ((uint32_t)record[4] << 24);

  if (record[5] != slot || record[6] != checksum(record[0], storedGeneration, slot)) {
    return false;
  }

  value = record[0];
  generation = storedGeneration;
  return true;
}

bool ReadSecuenceService::migrateLegacy() {
  if (!_fs) {
    return false;
  }

  File legacySeq = _fs->open(READ_SECUENCE_LEGACY_FILE, "r");
  if (legacySeq) {
    DynamicJsonDocument doc(64);
    const DeserializationError error = deserializeJson(doc, legacySeq);
    legacySeq.close();
    if (error == DeserializationError::Ok && doc.containsKey("read_secuence")) {
      _value = doc["read_secuence"] | _value;
      _generation = _value;
      return true;
    }
  }

  File legacyState = _fs->open(DEVICE_STATE_FILE, "r");
  if (!legacyState) {
    return false;
  }

  DynamicJsonDocument stateDoc(512);
  const DeserializationError stateError = deserializeJson(stateDoc, legacyState);
  legacyState.close();

  if (stateError == DeserializationError::Ok && stateDoc.containsKey("read_secuence")) {
    _value = stateDoc["read_secuence"] | _value;
    _generation = _value;
    return true;
  }

  return false;
}

bool ReadSecuenceService::scanSlots() {
  bool found = false;
  uint8_t bestSlot = 0;
  uint8_t bestValue = 0;
  uint32_t bestGeneration = 0;

  for (uint8_t slot = 0; slot < READ_SEQ_SLOT_COUNT; slot++) {
    uint8_t value = 0;
    uint32_t generation = 0;
    if (!readSlot(slot, value, generation)) {
      continue;
    }
    if (!found || generation > bestGeneration) {
      found = true;
      bestSlot = slot;
      bestValue = value;
      bestGeneration = generation;
    }
  }

  if (!found) {
    return false;
  }

  _activeSlot = bestSlot;
  _value = bestValue;
  _generation = bestGeneration;
  return true;
}

void ReadSecuenceService::readFromFS() {
  if (!_fs) {
    return;
  }

  ensureDirs();

  if (scanSlots()) {
    return;
  }

  if (migrateLegacy()) {
    _activeSlot = 0;
    writeSlot(_activeSlot, _value, _generation);
  }
}
