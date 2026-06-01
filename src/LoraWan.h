#ifndef LORA_WAN_H
#define LORA_WAN_H

#include <Arduino.h>
#include "Settings.h"  // define LORAWAN_RESPONSE_BUFFER aquí

//Doc for commands https://docs.rakwireless.com/product-categories/software-apis-and-libraries/rui3/at-command-manual/#lorawan-joining-and-sending

typedef void (*LoraWanRxLineHandler)(const String &line);

class LoraWan {
  public:
    LoraWan(HardwareSerial &serial);
    void begin(unsigned long baud = 9600);
    void enableATMode();

    void setRxLineHandler(LoraWanRxLineHandler handler);
    /** Lee serial, encola +EVT:RX_* y despacha cola. true si procesó al menos un RX. */
    bool poll();
    void processRxQueue();

    bool setATM();

    // Bajo consumo
    bool sleep(unsigned long ms);
    bool setLowPowerMode(bool enabled);
    bool setLowPowerLevel(uint8_t level); // 1 o 2

    //TX
    bool setClassMode(char mode); // 'A', 'B' o 'C'

    // OTAA
    bool setDevEUI(const char *eui);
    bool setAppEUI(const char *eui);
    bool setAppKey(const char *key);

    // ABP
    bool setDevAddr(const char *addr);
    bool setNwkSKey(const char *key);
    bool setAppSKey(const char *key);
    bool setNetID(const char *id);

    // Join y modo
    bool setJoinMode(bool otaa);
    bool join(uint8_t attempts = 8, uint8_t interval = 10, bool autoJoin = false);
    bool isJoined();

    // Envío y recepción
    bool sendHeartbeat();
    bool send(uint8_t port, const char *hexPayload, bool confirmed = false);
    int getLastConfirmStatus();
    bool getLastReceived(char *output, size_t maxLen);

    // Getters
    bool getDevEUI(char *out, size_t len);
    bool getAppEUI(char *out, size_t len);
    bool getAppKey(char *out, size_t len);
    bool getDevAddr(char *out, size_t len);
    bool getAppSKey(char *out, size_t len);
    bool getNwkSKey(char *out, size_t len);
    bool getNetID(char *out, size_t len);
    bool getJoinMode(char *out, size_t len);
    bool getJoinStatus(char *out, size_t len);
    bool getConfirmStatus(char *out, size_t len);
    bool getLowPowerMode(char *out, size_t len);
    bool getLowPowerLevel(char *out, size_t len);
    bool getClassMode(char *out, size_t len);

  private:
    HardwareSerial *_serial;
    LoraWanRxLineHandler _rxHandler;
    bool _processingRx;
    uint8_t _rxQueueCount;
    String _rxQueue[LORAWAN_RX_QUEUE_DEPTH];
    String _lineBuffer;

    bool sendCommand(const char *cmd);
    bool sendCommand(const char *cmd, const char *expected);
    bool sendCommand(const char *cmd, const char *expected, uint16_t timeout);
    bool sendUplink(const char *cmd, bool confirmed, uint16_t timeout);
    bool getResponse(const char *cmd, char *response, size_t maxLen, uint16_t timeout = 1000);
    void flushInput();
    void drainPendingSerial();
    void feedSerial();
    void ingestSerialByte(char c);
    void enqueueRxLine(const String &line);
};

#endif
