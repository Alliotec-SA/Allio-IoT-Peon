#ifndef SETTINGS_H
#define SETTINGS_H

#define LORAWAN_RESPONSE_BUFFER 128
#define LORAWAN_POST_TX_RX_MS 2500
#define LORAWAN_UPLINK_TIMEOUT_MS 5000
#define LORAWAN_RX_QUEUE_DEPTH 2
#define LORAWAN_MAX_RX_LINE_LEN 160
#define LORAWAN_DOWNLOAD_LINK_COMMANDS_FPORT 1
#define LORAWAN_TELEMETRY_FPORT 12
#define LORAWAN_UPLOAD_LINK_ACK_FPORT 222
#define LORAWAN_MAX_CMDS 5
#define LORAWAN_MAX_DATA_LEN 2
#define LORAWAN_RX_COMMANDS_VERSION 0x01
#define LORAWAN_TX_ACK_VERSION 0x01
#define LORAWAN_ATOMIC_EXECUTION_FLAG 0x01
#define LORAWAN_COMMANDS_CHECK_ELECTRIFIER_ON_STATE 0x00
#define LORAWAN_COMMANDS_TURN_ON_ELECTRIFIER 0x01
#define LORAWAN_COMMANDS_TURN_OFF_ELECTRIFIER 0x02
#define LORAWAN_COMMANDS_READ_ELECTRIFIER 0x03
#define LORAWAN_HEART_BEAT_INTERVAL_FOR_CLASS_A_IN_SECONDS 15 // 15
#define LORAWAN_HEART_BEAT_INTERVAL_FOR_CLASS_C_IN_SECONDS 1800  // 30 minutos

#define LORAWAN_DEFAULT_BAND 5
#define LORAWAN_DEFAULT_SUB_BAND 2
#define LORAWAN_DEFAULT_DATA_RATE 3
#define LORAWAN_DEFAULT_RX2_DR 8
#define LORAWAN_DEFAULT_RX2_FREQ_HZ 923300000UL
#define LORAWAN_DEFAULT_ADR true
#define LORAWAN_DEFAULT_CONFIRM_MODE false
#define LORAWAN_DEFAULT_USE_OTAA false
#define LORAWAN_DEFAULT_CLASS_MODE "A"

#define RTC_ADDR 65  // safe address range is 64–127

#define READING_TRIES 3
#define MAX_TIME_TO_START_SETUP_IN_SECONDS 120 
#define UPDATE_TIME_IN_HOURS 1
#define INTERNAL_WAKEUP_TO_CHECK_UPDATE_TIME_IN_MINUTES 30

#define SERIAL_BAUD_RATE 115200
#define PIN_SIGNAL 14
#define PIN_DRST 12
#define PIN_WKP 16
#define PIN_SETTINGS_MODE 13
#define PIN_RAK_RESET 15
#define PIN_TURN_ON_OFF_ELECTRIFIER 0
#define SIGNAL_FACTOR 2801
#define SIGNAL_TIMEOUT 6000
#define CHANNEL_SIGNAL 3
#define CHANNEL_BATTERY 1
#define CHANNEL_PANEL 2
#define CHANNEL_OWN_BATTERY 0
#define OWN_BATTERY_FACTOR 11.11 //(1)/(10/110)
#define BATTERY_FACTOR 11.11 //(1)/(10/110)
#define SOLAR_FACTOR 11.11 //(1)/(10/110)
#define UPDATE_TIME 60000



#include <Arduino.h>

struct RTCData {
  uint8_t wakeup_cycle;  // 0 or 1
};

struct LastResult {
    unsigned long signalPeriod;
    bool timeout;
    bool ready;
    unsigned long lastChecked;
    uint8_t batteryPercent;
    uint8_t readSecuence;
    float batteryVoltage;
    float solarVoltage;
    float signalVoltage;
    float battery;
  };


typedef struct {
  uint8_t cmd;
  uint8_t len;
  uint8_t data[LORAWAN_MAX_DATA_LEN];

  bool valid;
  bool executed;
  bool success;
} LoRaWanRxCommand;

typedef struct {
  uint8_t version;
  bool atomicExecution;
} LoRaWanRxFlags;

typedef struct {
  LoRaWanRxFlags   flags;
  LoRaWanRxCommand cmds[LORAWAN_MAX_CMDS];
  uint8_t   cmdCount;
} LoRaWanDownlinkContext;


#endif