#ifndef ReadSecuenceService_h
#define ReadSecuenceService_h

#include <Arduino.h>
#include <FS.h>

#define READ_SECUENCE_FILE "/config/readSecuence.json"

class ReadSecuenceService {
 public:
  void begin(FS* fs);
  uint8_t get() const;
  uint8_t increment();

 private:
  FS* _fs;
  uint8_t _value;

  void readFromFS();
  bool writeToFS();
  void ensureConfigDir();
};

extern ReadSecuenceService readSecuenceService;

#endif
