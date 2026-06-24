#ifndef ReadSecuenceService_h
#define ReadSecuenceService_h

#include <Arduino.h>
#include <FS.h>

#define READ_SEQ_DIR "/config/readSeq"
#define READ_SEQ_SLOT_COUNT 8
#define READ_SEQ_RECORD_SIZE 9
#define READ_SEQ_RECORD_VERSION 0x02
#define READ_SECUENCE_LEGACY_FILE "/config/readSecuence.json"

class ReadSecuenceService {
 public:
  void begin(FS* fs);
  uint8_t get() const;
  uint8_t increment();

 private:
  FS* _fs;
  uint8_t _value;
  uint8_t _activeSlot;
  uint32_t _generation;

  void readFromFS();
  bool scanSlots();
  bool writeSlot(uint8_t slot, uint8_t value, uint32_t generation);
  bool readSlot(uint8_t slot, uint8_t& value, uint32_t& generation) const;
  void ensureDirs();
  bool migrateLegacy();
  static uint8_t checksum(uint8_t value, uint32_t generation, uint8_t slot);
};

extern ReadSecuenceService readSecuenceService;

#endif
