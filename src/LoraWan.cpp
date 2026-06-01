#include "LoraWan.h"

LoraWan::LoraWan(HardwareSerial &serial) {
  _serial = &serial;
  _rxHandler = nullptr;
  _processingRx = false;
  _rxQueueCount = 0;
}

void LoraWan::setRxLineHandler(LoraWanRxLineHandler handler) {
  _rxHandler = handler;
}

void LoraWan::enqueueRxLine(const String &line) {
  if (_rxQueueCount >= LORAWAN_RX_QUEUE_DEPTH) {
    SerialDebug.println("LoRaWAN RX queue full, dropping oldest");
    for (uint8_t i = 0; i < LORAWAN_RX_QUEUE_DEPTH - 1; i++) {
      _rxQueue[i] = _rxQueue[i + 1];
    }
    _rxQueueCount = LORAWAN_RX_QUEUE_DEPTH - 1;
  }
  _rxQueue[_rxQueueCount++] = line;
}

void LoraWan::ingestSerialByte(char c) {
  if (c == '\r') {
    return;
  }
  if (c == '\n') {
    if (_lineBuffer.length() > 0) {
      _lineBuffer.trim();
      if (_lineBuffer.startsWith("+EVT:RX_")) {
        SerialDebug.print("LoRaWAN RX queued: ");
        SerialDebug.println(_lineBuffer);
        enqueueRxLine(_lineBuffer);
      }
      _lineBuffer = "";
    }
    return;
  }
  if (_lineBuffer.length() < LORAWAN_MAX_RX_LINE_LEN) {
    _lineBuffer += c;
  }
}

void LoraWan::feedSerial() {
  while (_serial->available()) {
    ingestSerialByte(static_cast<char>(_serial->read()));
  }
}

void LoraWan::drainPendingSerial() {
  feedSerial();
}

void LoraWan::processRxQueue() {
  if (_processingRx || !_rxHandler) {
    return;
  }
  _processingRx = true;
  while (_rxQueueCount > 0) {
    String line = _rxQueue[0];
    for (uint8_t i = 0; i < _rxQueueCount - 1; i++) {
      _rxQueue[i] = _rxQueue[i + 1];
    }
    _rxQueueCount--;
    _rxHandler(line);
  }
  _processingRx = false;
}

bool LoraWan::poll() {
  if (!_serial->available() && _rxQueueCount == 0) {
    return false;
  }
  feedSerial();
  bool hadRx = _rxQueueCount > 0;
  processRxQueue();
  feedSerial();
  if (_rxQueueCount > 0) {
    hadRx = true;
    processRxQueue();
  }
  return hadRx;
}

void LoraWan::begin(unsigned long baud) {
  _serial->begin(baud);
  flushInput();
}

void LoraWan::enableATMode() {
  sendCommand("AT+ATM"); // Enable AT mode for LoRaWAN
}

void LoraWan::flushInput() {
  while (_serial->available()) {
    _serial->read();
  }
  _lineBuffer = "";
}

bool LoraWan::sendCommand(const char *cmd) {
  return sendCommand(cmd, "OK", 2000);
}

bool LoraWan::sendCommand(const char *cmd, const char *expected) {
  return sendCommand(cmd, expected, 2000);
}

bool LoraWan::sendCommand(const char *cmd, const char *expected, uint16_t timeout) {
  SerialDebug.print("LoRaWAN send command: ");
  SerialDebug.println(cmd);
  drainPendingSerial();
  _serial->println(cmd);
  char response[LORAWAN_RESPONSE_BUFFER] = {0};
  size_t i = 0;
  unsigned long start = millis();
  bool success = false;
  while (millis() - start < timeout && i < sizeof(response) - 1) {
    while (_serial->available() && i < sizeof(response) - 1) {
      char c = static_cast<char>(_serial->read());
      response[i++] = c;
      ingestSerialByte(c);
    }
    response[i] = '\0';
    if (strstr(response, expected)) {
      success = true;
      break;
    }
    if (strstr(response, "ERROR") || strstr(response, "AT_")) {
      SerialDebug.print("LoRaWAN command error response: ");
      SerialDebug.println(response);
      drainPendingSerial();
      processRxQueue();
      return false;
    }
  }
  if (!success) {
    SerialDebug.print("LoRaWAN command timeout response: ");
    SerialDebug.println(response);
  }
  drainPendingSerial();
  processRxQueue();
  return success;
}

bool LoraWan::sendUplink(const char *cmd, bool confirmed, uint16_t timeout) {
  SerialDebug.print("LoRaWAN send command: ");
  SerialDebug.println(cmd);
  drainPendingSerial();
  _serial->println(cmd);

  const char *expected = confirmed ? "+EVT:SEND CONFIRMED OK" : "+EVT:TX_DONE";
  char response[LORAWAN_RESPONSE_BUFFER] = {0};
  size_t i = 0;
  unsigned long start = millis();
  bool success = false;

  while (millis() - start < timeout && i < sizeof(response) - 1) {
    while (_serial->available() && i < sizeof(response) - 1) {
      char c = static_cast<char>(_serial->read());
      response[i++] = c;
      ingestSerialByte(c);
    }
    response[i] = '\0';
    if (strstr(response, expected)) {
      success = true;
      break;
    }
    if (strstr(response, "ERROR") || strstr(response, "AT_")) {
      SerialDebug.print("LoRaWAN command error response: ");
      SerialDebug.println(response);
      drainPendingSerial();
      processRxQueue();
      return false;
    }
  }

  if (!success) {
    SerialDebug.print("LoRaWAN uplink timeout response: ");
    SerialDebug.println(response);
  }

  unsigned long postStart = millis();
  while (millis() - postStart < LORAWAN_POST_TX_RX_MS) {
    feedSerial();
    yield();
  }

  drainPendingSerial();
  processRxQueue();
  return success;
}

bool LoraWan::getResponse(const char *cmd, char *response, size_t maxLen, uint16_t timeout) {
  drainPendingSerial();
  _serial->println(cmd);
  size_t i = 0;
  unsigned long start = millis();
  while (millis() - start < timeout && i < maxLen - 1) {
    while (_serial->available() && i < maxLen - 1) {
      char c = static_cast<char>(_serial->read());
      response[i++] = c;
      ingestSerialByte(c);
    }
  }
  response[i] = '\0';
  drainPendingSerial();
  processRxQueue();
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

bool LoraWan::setClassMode(char mode) {
  if (mode != 'A' && mode != 'B' && mode != 'C') {
    SerialDebug.println("Invalid class mode. Use 'A', 'B', or 'C'.");
    return false;
  }
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "AT+CLASS=%c", mode);
  return sendCommand(cmd);
}

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

bool LoraWan::setATM() {
  return sendCommand("AT+ATM");
}

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

bool LoraWan::sendHeartbeat() {
  return send(223, "00", false);
}

bool LoraWan::send(uint8_t port, const char *hexPayload, bool confirmed) {
  char sendCmd[256];
  snprintf(sendCmd, sizeof(sendCmd), "AT+SEND=%d:%s", port, hexPayload);
  return sendUplink(sendCmd, confirmed, LORAWAN_UPLINK_TIMEOUT_MS);
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
bool LoraWan::getClassMode(char *out, size_t len)     { return getResponse("AT+CLASS=?", out, len); }
