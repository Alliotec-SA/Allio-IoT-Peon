#include "LoraWan.h"

LoraWan::LoraWan(HardwareSerial &serial) {
  _serial = &serial;
}

void LoraWan::begin(unsigned long baud) {
  _serial->begin(baud);
  flushInput();
}

void LoraWan::flushInput() {
  while (_serial->available()) _serial->read();
}

bool LoraWan::sendCommand(const char *cmd, const char *expected, uint16_t timeout) {
  _serial->println(cmd);
  char response[LORAWAN_RESPONSE_BUFFER] = {0};
  size_t i = 0;
  unsigned long start = millis();
  while (millis() - start < timeout && i < sizeof(response) - 1) {
    while (_serial->available() && i < sizeof(response) - 1) {
      response[i++] = _serial->read();
    }
    response[i] = '\0';
    if (strstr(response, expected)) return true;
    if (strstr(response, "ERROR") || strstr(response, "AT_")) return false;
  }
  return false;
}

bool LoraWan::getResponse(const char *cmd, char *response, size_t maxLen, uint16_t timeout) {
  flushInput();
  _serial->println(cmd);
  size_t i = 0;
  unsigned long start = millis();
  while (millis() - start < timeout && i < maxLen - 1) {
    while (_serial->available() && i < maxLen - 1) {
      response[i++] = _serial->read();
    }
  }
  response[i] = '\0';
  return i > 0;
}

// Sleep y bajo consumo
bool LoraWan::sleep(unsigned long ms) {
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+SLEEP=%lu", ms);
  return sendCommand(cmd);
}

bool LoraWan::setLowPowerMode(bool enabled) {
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "AT+LPM=%d", enabled ? 1 : 0);
  return sendCommand(cmd);
}

bool LoraWan::setLowPowerLevel(uint8_t level) {
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "AT+LPMLVL=%d", level);
  return sendCommand(cmd);
}

// OTAA
bool LoraWan::setDevEUI(const char *eui) {
  char cmd[40];
  snprintf(cmd, sizeof(cmd), "AT+DEVEUI=%s", eui);
  return sendCommand(cmd);
}

bool LoraWan::setAppEUI(const char *eui) {
  char cmd[40];
  snprintf(cmd, sizeof(cmd), "AT+APPEUI=%s", eui);
  return sendCommand(cmd);
}

bool LoraWan::setAppKey(const char *key) {
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "AT+APPKEY=%s", key);
  return sendCommand(cmd);
}

// ABP
bool LoraWan::setDevAddr(const char *addr) {
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+DEVADDR=%s", addr);
  return sendCommand(cmd);
}

bool LoraWan::setAppSKey(const char *key) {
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "AT+APPSKEY=%s", key);
  return sendCommand(cmd);
}

bool LoraWan::setNwkSKey(const char *key) {
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "AT+NWKSKEY=%s", key);
  return sendCommand(cmd);
}

bool LoraWan::setNetID(const char *id) {
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+NETID=%s", id);
  return sendCommand(cmd);
}

// Join y modo
bool LoraWan::setJoinMode(bool otaa) {
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "AT+NJM=%d", otaa ? 1 : 0);
  return sendCommand(cmd);
}

bool LoraWan::join(uint8_t attempts, uint8_t interval, bool autoJoin) {
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+JOIN=1:%d:%d:%d", autoJoin ? 1 : 0, interval, attempts);
  return sendCommand(cmd, "+EVT:JOINED", 8000);
}

bool LoraWan::isJoined() {
  char response[LORAWAN_RESPONSE_BUFFER];
  if (getResponse("AT+NJS=?", response, sizeof(response))) {
    return strstr(response, "=1") != nullptr;
  }
  return false;
}

// Envío y recepción
bool LoraWan::send(uint8_t port, const char *hexPayload, bool confirmed) {
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "AT+CFM=%d", confirmed ? 1 : 0);
  //if (!sendCommand(cmd)) return false;

  char sendCmd[256];
  snprintf(sendCmd, sizeof(sendCmd), "AT+SEND=%d:%s", port, hexPayload);
  return sendCommand(sendCmd, "+EVT:SEND CONFIRMED OK", 5000);
}

int LoraWan::getLastConfirmStatus() {
  char response[LORAWAN_RESPONSE_BUFFER];
  if (getResponse("AT+CFS=?", response, sizeof(response))) {
    if (strstr(response, "=1")) return 1;
    if (strstr(response, "=0")) return 0;
  }
  return -1;
}

bool LoraWan::getLastReceived(char *output, size_t maxLen) {
  return getResponse("AT+RECV=?", output, maxLen);
}

// Getters
bool LoraWan::getDevEUI(char *out, size_t len)        { return getResponse("AT+DEVEUI=?", out, len); }
bool LoraWan::getAppEUI(char *out, size_t len)        { return getResponse("AT+APPEUI=?", out, len); }
bool LoraWan::getAppKey(char *out, size_t len)        { return getResponse("AT+APPKEY=?", out, len); }
bool LoraWan::getDevAddr(char *out, size_t len)       { return getResponse("AT+DEVADDR=?", out, len); }
bool LoraWan::getAppSKey(char *out, size_t len)       { return getResponse("AT+APPSKEY=?", out, len); }
bool LoraWan::getNwkSKey(char *out, size_t len)       { return getResponse("AT+NWKSKEY=?", out, len); }
bool LoraWan::getNetID(char *out, size_t len)         { return getResponse("AT+NETID=?", out, len); }
bool LoraWan::getJoinMode(char *out, size_t len)      { return getResponse("AT+NJM=?", out, len); }
bool LoraWan::getJoinStatus(char *out, size_t len)    { return getResponse("AT+NJS=?", out, len); }
bool LoraWan::getConfirmStatus(char *out, size_t len) { return getResponse("AT+CFS=?", out, len); }
bool LoraWan::getLowPowerMode(char *out, size_t len)  { return getResponse("AT+LPM=?", out, len); }
bool LoraWan::getLowPowerLevel(char *out, size_t len) { return getResponse("AT+LPMLVL=?", out, len); }
