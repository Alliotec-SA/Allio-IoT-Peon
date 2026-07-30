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
/** Cooldown after an accepted manual/remote read request. Must exceed the worst-case read cycle
 *  (READING_TRIES attempts of up to 2 x SIGNAL_TIMEOUT each) so a client that keeps re-issuing the
 *  command cannot abort the cycle before it settles, which would leave the device never
 *  transmitting. */
#define MANUAL_READ_MIN_INTERVAL_MS 60000UL
/** Spacing between retries of the same transport. A LoRaWAN uplink blocks for seconds and the
 *  band has duty cycle limits, so retries are few and well separated rather than tight. */
#define SEND_RETRY_INTERVAL_MS 60000UL
/** Absolute cap on how long a reading may wait for its transports. Past this the pending channels
 *  are abandoned so the cycle closes and the next reading starts from a clean state. */
#define SEND_GIVE_UP_MS 300000UL
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
/** Bound on how long to wait for one ADS1115 conversion. A conversion takes ~2.1ms at 475SPS, so
 *  this is never reached in normal operation: it exists because the driver's own wait loop has no
 *  timeout and spins forever if the I2C bus locks up, which the electrifier's EMI can cause.
 *  Deliberately polled without yield() so the sampling period stays tight and predictable. */
#define ADC_READ_TIMEOUT_MS 50UL
/** Retries for one-off voltage readings, which run outside the edge-detection loop and can afford
 *  another attempt rather than reporting a bogus zero. */
#define ADC_READ_ATTEMPTS 3
/** Faster bus means each transaction spends less time exposed to the electrifier's noise, but also
 *  less margin against it. Watch the dropped-sample count reported per attempt: if it climbs, this
 *  is the first value to walk back (200000 keeps half the gain with more margin). */
#define I2C_CLOCK_HZ 400000UL
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