
#include <Functions.h>
#include "Settings.h"
#include <ESP8266React.h>
#include <Adafruit_ADS1X15.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <DeviceStateService.h>

extern Adafruit_ADS1115 ads;

/** Same as Adafruit_ADS1X15::readADC_SingleEnded but bounded. The library version waits on
 *  `while (!conversionComplete());` with no timeout, and conversionComplete() reads a register
 *  whose I2C read result is discarded, so a locked bus makes it spin forever. */
static bool readAdcCounts(uint8_t channel, int16_t &out) {
  if (channel > 3) {
    return false;
  }
  ads.startADCReading(MUX_BY_CHANNEL[channel], /*continuous=*/false);
  const unsigned long start = millis();
  while (!ads.conversionComplete()) {
    if (millis() - start > ADC_READ_TIMEOUT_MS) {
      return false;
    }
  }
  out = ads.getLastConversionResults();
  return true;
}

float getVoltage(uint8_t channel){
  int16_t val = 0;
  for (uint8_t attempt = 0; attempt < ADC_READ_ATTEMPTS; attempt++) {
    if (readAdcCounts(channel, val)) {
      return ads.computeVolts(val);
    }
    SerialDebug.print("ADC read timed out on channel ");
    SerialDebug.println(channel);
  }
  return 0.0f;
}

float getSignalVp(float voltage){
  return voltage*SIGNAL_FACTOR;
}

float getOwnBatteryVoltage(){
  return getVoltage(CHANNEL_OWN_BATTERY)*OWN_BATTERY_FACTOR;
}

float getBatteryVoltage(){
  return getVoltage(CHANNEL_BATTERY)*BATTERY_FACTOR;
}

/** Resting-voltage to state-of-charge points for a 12V lead-acid battery, lowest first.
 *  The relationship is not linear: the top quarter spans 0.30V while each lower quarter spans
 *  0.20V, so a straight line between the endpoints overstates charge across the middle. */
struct BatteryCurvePoint {
  float volts;
  uint8_t percent;
};

static const BatteryCurvePoint BATTERY_CURVE[] = {
    {11.80f, 0},
    {12.00f, 25},
    {12.20f, 50},
    {12.40f, 75},
    {12.70f, 100},
};

uint8_t getBatteryPercent(float volts) {
  const size_t count = sizeof(BATTERY_CURVE) / sizeof(BATTERY_CURVE[0]);

  if (volts <= BATTERY_CURVE[0].volts) {
    return BATTERY_CURVE[0].percent;
  }

  for (size_t i = 1; i < count; i++) {
    if (volts <= BATTERY_CURVE[i].volts) {
      const BatteryCurvePoint &low = BATTERY_CURVE[i - 1];
      const BatteryCurvePoint &high = BATTERY_CURVE[i];
      const float ratio = (volts - low.volts) / (high.volts - low.volts);
      // +0.5 so the value rounds instead of always truncating downwards.
      return (uint8_t)(low.percent + ratio * (high.percent - low.percent) + 0.5f);
    }
  }

  // Above the last point: charging or fully charged.
  return BATTERY_CURVE[count - 1].percent;
}

float getSolarPannelVoltage(){
  return getVoltage(CHANNEL_PANEL)*SOLAR_FACTOR;
}

bool hasClientConnected(){
  return WiFi.softAPgetStationNum() > 0;
}


void ackCommandOnActiveChannels(const String &httpAction, uint8_t loraCmd, boolean executionCommandDone, const String &httpResponse) {
  if (deviceSettingsService.isEnabled()) {
    if (WiFi.status() == WL_CONNECTED) {
      ackCommandPost(
          deviceSettingsService.getServer(),
          deviceSettingsService.getPath(),
          deviceSettingsService.getToken(),
          deviceSettingsService.getDevEUI(),
          httpAction,
          executionCommandDone,
          httpResponse);
    } else {
      SerialDebug.println("ACK HTTP skipped: not connected to WiFi");
    }
  }

  if (deviceLoRaWanSettingsService.isEnabled()) {
    sendLoRaWanCommandACK(loraCmd, executionCommandDone);
  }
}

void turnOnElectrifier(boolean state, boolean *isTurnedOn, DeviceStateService *deviceStateService, const ElectrifierAckContext *ack) {
  *isTurnedOn = state;
  deviceStateService->updateElectrifierState(state);
  if (state) {
    digitalWrite(PIN_TURN_ON_OFF_ELECTRIFIER, HIGH);
    SerialDebug.println("Electrifier Turned ON");
  } else {
    digitalWrite(PIN_TURN_ON_OFF_ELECTRIFIER, LOW);
    SerialDebug.println("Electrifier Turned OFF");
  }

  if (ack != nullptr) {
    ackCommandOnActiveChannels(
        String(ack->httpAction),
        ack->loraCmd,
        ack->success,
        String(ack->response));
  }
}

void resetWifiSettings() {
  esp8266React.getWiFiSettingsService()->update([](WiFiSettings& s) {
    s.ssid = "";
    s.password = "";
    return StateUpdateResult::CHANGED;
  }, "reset");
}

void goToSleep(unsigned long t0){
  SerialDebug.println("Preparing to sleep");

  lorawan.setLowPowerMode(true); // Dormir módulo externo       
  delay(100);               // Dejar que termine comunicación serial
  
  SerialDebug.println("Call elapsed time");
  unsigned long elapsed = millis() - t0;
  unsigned long targetMs = INTERNAL_WAKEUP_TO_CHECK_UPDATE_TIME_IN_MINUTES * 60UL * 1000UL;
  unsigned long sleepMs;

  if (elapsed >= targetMs) {
    sleepMs = 60UL * 1000UL;
    Serial.println("Time exceeded, using 1-minute fallback.");
  } else {
    sleepMs = targetMs - elapsed;
  }

  SerialDebug.print("Sleep ms: ");
  SerialDebug.println(sleepMs);

  uint64_t sleepUs = (uint64_t)sleepMs * 1000ULL;

  // Validar valor antes de usarlo
  if (sleepUs == 0 || sleepUs > 4294967295ULL) {
    SerialDebug.println("Invalid sleepUs, forcing fallback.");
    sleepUs = 60ULL * 1000ULL * 1000ULL; // 1 minuto
  }

  SerialDebug.print("Sleep us: ");
  SerialDebug.println((uint32_t)sleepUs);

  SerialDebug.println("Going to sleep...");
  SerialDebug.println("Stop everything");

  delay(200);

  // Before deep sleep , stop evertiting:
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  SerialDebug.flush();
  Serial.flush();
  delay(50);

  ESP.deepSleep((uint32_t)sleepUs, WAKE_RF_DEFAULT); // o WAKE_RF_DISABLED
}

static bool loraWanBandUsesChannelMask(uint8_t band) {
  return band == 1 || band == 5 || band == 6;
}

static String subBandToChannelMaskHex(uint8_t subBand) {
  static const uint16_t masks[] = {
      0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100};
  if (subBand == 0 || subBand > 9) {
    return "0000";
  }
  char buf[5];
  snprintf(buf, sizeof(buf), "%04X", masks[subBand]);
  return String(buf);
}

static String normalizeHexString(const String &value) {
  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (c == ':' || c == ' ' || c == '\r' || c == '\n') {
      continue;
    }
    if (c >= 'a' && c <= 'f') {
      c = static_cast<char>(c - 32);
    }
    out += c;
  }
  return out;
}

static String normalizeMaskHex(const String &value) {
  String hex = normalizeHexString(value);
  while (hex.length() < 4) {
    hex = "0" + hex;
  }
  if (hex.length() > 4) {
    hex = hex.substring(hex.length() - 4);
  }
  return hex;
}

static bool parseAtIntValue(const char *response, int &out) {
  if (response == nullptr || response[0] == '\0') {
    return false;
  }
  const char *p = strrchr(response, '=');
  if (p == nullptr) {
    p = strrchr(response, ':');
  }
  if (p == nullptr) {
    return false;
  }
  p++;
  while (*p == ' ') {
    p++;
  }
  char *end = nullptr;
  long value = strtol(p, &end, 10);
  if (end == p) {
    return false;
  }
  out = static_cast<int>(value);
  return true;
}

static String parseAtHexValue(const char *response) {
  if (response == nullptr || response[0] == '\0') {
    return "";
  }
  const char *p = strrchr(response, '=');
  if (p == nullptr) {
    p = strrchr(response, ':');
  }
  if (p == nullptr) {
    return normalizeHexString(String(response));
  }
  p++;
  while (*p == ' ') {
    p++;
  }
  return normalizeHexString(String(p));
}

static char parseAtClassValue(const char *response) {
  if (response == nullptr) {
    return '\0';
  }
  for (const char *p = response; *p != '\0'; p++) {
    if (*p == 'A' || *p == 'B' || *p == 'C') {
      return *p;
    }
  }
  return '\0';
}

static bool readRakBand(char *out, size_t len) { return lorawan.getBand(out, len); }
static bool readRakMask(char *out, size_t len) { return lorawan.getChannelMask(out, len); }
static bool readRakJoinMode(char *out, size_t len) { return lorawan.getJoinMode(out, len); }
static bool readRakClass(char *out, size_t len) { return lorawan.getClassMode(out, len); }
static bool readRakDevEui(char *out, size_t len) { return lorawan.getDevEUI(out, len); }
static bool readRakAppEui(char *out, size_t len) { return lorawan.getAppEUI(out, len); }
static bool readRakAppKey(char *out, size_t len) { return lorawan.getAppKey(out, len); }
static bool readRakDevAddr(char *out, size_t len) { return lorawan.getDevAddr(out, len); }
static bool readRakAppSKey(char *out, size_t len) { return lorawan.getAppSKey(out, len); }
static bool readRakNwkSKey(char *out, size_t len) { return lorawan.getNwkSKey(out, len); }
static bool readRakRx2Dr(char *out, size_t len) { return lorawan.getRx2Dr(out, len); }
static bool readRakRx2Freq(char *out, size_t len) { return lorawan.getRx2Freq(out, len); }
static bool readRakAdr(char *out, size_t len) { return lorawan.getAdr(out, len); }
static bool readRakDr(char *out, size_t len) { return lorawan.getDataRate(out, len); }
static bool readRakCfm(char *out, size_t len) { return lorawan.getConfirmMode(out, len); }
static bool readRakLpm(char *out, size_t len) { return lorawan.getLowPowerMode(out, len); }

static bool compareIntField(const char *name, int expected, bool (*getter)(char *, size_t), bool &mismatch, bool joinCritical) {
  char response[LORAWAN_RESPONSE_BUFFER] = {0};
  if (!getter(response, sizeof(response))) {
    SerialDebug.print("LoRaWAN sync: failed to read ");
    SerialDebug.println(name);
    mismatch = true;
    return joinCritical;
  }
  int actual = -1;
  if (!parseAtIntValue(response, actual)) {
    SerialDebug.print("LoRaWAN sync: failed to parse ");
    SerialDebug.println(name);
    mismatch = true;
    return joinCritical;
  }
  if (actual != expected) {
    SerialDebug.print("LoRaWAN sync mismatch ");
    SerialDebug.print(name);
    SerialDebug.print(": expected=");
    SerialDebug.print(expected);
    SerialDebug.print(" actual=");
    SerialDebug.println(actual);
    mismatch = true;
    return joinCritical;
  }
  return false;
}

static bool compareHexField(const char *name, const String &expected, bool (*getter)(char *, size_t), bool &mismatch, bool joinCritical) {
  if (expected.length() == 0) {
    return false;
  }
  char response[LORAWAN_RESPONSE_BUFFER] = {0};
  if (!getter(response, sizeof(response))) {
    SerialDebug.print("LoRaWAN sync: failed to read ");
    SerialDebug.println(name);
    mismatch = true;
    return joinCritical;
  }
  String actual = parseAtHexValue(response);
  if (actual != normalizeHexString(expected)) {
    SerialDebug.print("LoRaWAN sync mismatch ");
    SerialDebug.print(name);
    SerialDebug.print(": expected=");
    SerialDebug.print(expected);
    SerialDebug.print(" actual=");
    SerialDebug.println(actual);
    mismatch = true;
    return joinCritical;
  }
  return false;
}

static bool compareClassField(const char *name, char expected, bool (*getter)(char *, size_t), bool &mismatch) {
  char response[LORAWAN_RESPONSE_BUFFER] = {0};
  if (!getter(response, sizeof(response))) {
    SerialDebug.print("LoRaWAN sync: failed to read ");
    SerialDebug.println(name);
    mismatch = true;
    return false;
  }
  char actual = parseAtClassValue(response);
  if (actual != expected) {
    SerialDebug.print("LoRaWAN sync mismatch ");
    SerialDebug.print(name);
    SerialDebug.print(": expected=");
    SerialDebug.print(expected);
    SerialDebug.print(" actual=");
    SerialDebug.println(actual);
    mismatch = true;
  }
  return false;
}

void applyLoRaWanFlashConfig() {
  const uint8_t band = deviceLoRaWanSettingsService.getBand();
  const bool useOtaa = deviceLoRaWanSettingsService.shouldUseOtaa();
  const String classMode = deviceLoRaWanSettingsService.getClassMode();

  lorawan.setATM();
  lorawan.setBand(band);
  if (loraWanBandUsesChannelMask(band)) {
    const String maskHex = subBandToChannelMaskHex(deviceLoRaWanSettingsService.getSubBand());
    lorawan.setChannelMask(maskHex.c_str());
  }
  lorawan.setJoinMode(useOtaa);
  if (classMode.length() > 0) {
    lorawan.setClassMode(classMode.charAt(0));
  }

  if (useOtaa) {
    const String devEui = deviceLoRaWanSettingsService.getDevEUI();
    const String appEui = deviceLoRaWanSettingsService.getAppEUI();
    const String appKey = deviceLoRaWanSettingsService.getAppKey();
    if (devEui.length() > 0) {
      lorawan.setDevEUI(devEui.c_str());
    }
    if (appEui.length() > 0) {
      lorawan.setAppEUI(appEui.c_str());
    }
    if (appKey.length() > 0) {
      lorawan.setAppKey(appKey.c_str());
    }
  } else {
    const String devAddr = deviceLoRaWanSettingsService.getDevAddress();
    const String appSKey = deviceLoRaWanSettingsService.getAppsKey();
    const String nwkSKey = deviceLoRaWanSettingsService.getNetsKey();
    if (devAddr.length() > 0) {
      lorawan.setDevAddr(devAddr.c_str());
    }
    if (appSKey.length() > 0) {
      lorawan.setAppSKey(appSKey.c_str());
    }
    if (nwkSKey.length() > 0) {
      lorawan.setNwkSKey(nwkSKey.c_str());
    }
  }

  lorawan.setRx2Dr(deviceLoRaWanSettingsService.getRx2Dr());
  lorawan.setRx2Freq(deviceLoRaWanSettingsService.getRx2FreqHz());
  lorawan.setAdr(deviceLoRaWanSettingsService.getAdr());
  if (!deviceLoRaWanSettingsService.getAdr()) {
    lorawan.setDataRate(deviceLoRaWanSettingsService.getDataRate());
  }
  lorawan.setConfirmMode(deviceLoRaWanSettingsService.getConfirmMode());
  lorawan.setLowPowerMode(false);
}

void setupLoRaWan() {
  applyLoRaWanFlashConfig();
}

bool syncLoRaWanFromFlash() {
  bool mismatch = false;
  bool requiresJoin = false;

  const uint8_t band = deviceLoRaWanSettingsService.getBand();
  const bool useOtaa = deviceLoRaWanSettingsService.shouldUseOtaa();
  const String classMode = deviceLoRaWanSettingsService.getClassMode();
  const char expectedClass = classMode.length() > 0 ? classMode.charAt(0) : 'A';

  if (compareIntField("BAND", band, readRakBand, mismatch, true)) {
    requiresJoin = true;
  }

  if (loraWanBandUsesChannelMask(band)) {
    const String expectedMask = subBandToChannelMaskHex(deviceLoRaWanSettingsService.getSubBand());
    char response[LORAWAN_RESPONSE_BUFFER] = {0};
    if (!readRakMask(response, sizeof(response))) {
      SerialDebug.println("LoRaWAN sync: failed to read MASK");
      mismatch = true;
      requiresJoin = true;
    } else {
      const String actualMask = normalizeMaskHex(parseAtHexValue(response));
      if (actualMask != expectedMask) {
        SerialDebug.print("LoRaWAN sync mismatch MASK: expected=");
        SerialDebug.print(expectedMask);
        SerialDebug.print(" actual=");
        SerialDebug.println(actualMask);
        mismatch = true;
        requiresJoin = true;
      }
    }
  }

  if (compareIntField("NJM", useOtaa ? 1 : 0, readRakJoinMode, mismatch, true)) {
    requiresJoin = true;
  }
  compareClassField("CLASS", expectedClass, readRakClass, mismatch);

  if (useOtaa) {
    if (compareHexField("DEVEUI", deviceLoRaWanSettingsService.getDevEUI(), readRakDevEui, mismatch, true)) {
      requiresJoin = true;
    }
    if (compareHexField("APPEUI", deviceLoRaWanSettingsService.getAppEUI(), readRakAppEui, mismatch, true)) {
      requiresJoin = true;
    }
    if (compareHexField("APPKEY", deviceLoRaWanSettingsService.getAppKey(), readRakAppKey, mismatch, true)) {
      requiresJoin = true;
    }
  } else {
    if (compareHexField("DEVADDR", deviceLoRaWanSettingsService.getDevAddress(), readRakDevAddr, mismatch, true)) {
      requiresJoin = true;
    }
    if (compareHexField("APPSKEY", deviceLoRaWanSettingsService.getAppsKey(), readRakAppSKey, mismatch, true)) {
      requiresJoin = true;
    }
    if (compareHexField("NWKSKEY", deviceLoRaWanSettingsService.getNetsKey(), readRakNwkSKey, mismatch, true)) {
      requiresJoin = true;
    }
  }

  compareIntField("RX2DR", deviceLoRaWanSettingsService.getRx2Dr(), readRakRx2Dr, mismatch, false);
  compareIntField("RX2FQ", static_cast<int>(deviceLoRaWanSettingsService.getRx2FreqHz()), readRakRx2Freq, mismatch, false);

  const bool expectedAdr = deviceLoRaWanSettingsService.getAdr();
  compareIntField("ADR", expectedAdr ? 1 : 0, readRakAdr, mismatch, false);
  if (!expectedAdr) {
    compareIntField("DR", deviceLoRaWanSettingsService.getDataRate(), readRakDr, mismatch, false);
  }
  compareIntField("CFM", deviceLoRaWanSettingsService.getConfirmMode() ? 1 : 0, readRakCfm, mismatch, false);
  compareIntField("LPM", 0, readRakLpm, mismatch, false);

  if (mismatch) {
    SerialDebug.println("LoRaWAN sync: applying flash configuration to module");
    applyLoRaWanFlashConfig();
  } else {
    SerialDebug.println("LoRaWAN sync: OK");
  }

  if (deviceLoRaWanSettingsService.isEnabled() && useOtaa && (requiresJoin || !lorawan.isJoined())) {
    SerialDebug.println("LoRaWAN sync: joining network (OTAA)");
    lorawan.join();
  }

  return true;
}

void testBoardVoltageElement(Stream &port){
  port.print("Battery Voltage: "); 
  port.print(getBatteryVoltage());
  port.print("v\tSolar Voltage: "); 
  port.print(getSolarPannelVoltage());
  port.print("v\tOwn Battery Voltage: "); 
  port.print(getOwnBatteryVoltage());
  port.println("v");
}

/*
void sendJsonPost(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setSNIHostname(host.c_str());

  SerialDebug.println("[🔌 Intentando conexión]");
  SerialDebug.print("Host: ");
  SerialDebug.println(host);
  SerialDebug.print("Puerto: 443\n");
  SerialDebug.print("Path: ");
  SerialDebug.println(path);

 if (!client.connect(host.c_str(), 443)) {
    SerialDebug.println("❌ ERROR: No se pudo conectar al servidor HTTPS.");
    SerialDebug.println("Posibles causas:");
    SerialDebug.println("- ❗ Nombre del host mal escrito o incorrecto");
    SerialDebug.println("- ❗ El servidor está fuera de línea o no responde por el puerto 443");
    SerialDebug.println("- ❗ Problemas de DNS o red WiFi inestable");
    SerialDebug.println("- ❗ El certificado SSL no es válido (aunque usamos setInsecure)");

    // Diagnóstico adicional: verificar IP
    IPAddress ip;
    if (WiFi.hostByName(host.c_str(), ip)) {
        SerialDebug.print("🧭 Dirección IP resuelta: ");
        SerialDebug.println(ip);
        SerialDebug.println("➡️  El problema NO es DNS.");
    } else {
        SerialDebug.println("❌ ERROR: No se pudo resolver el host DNS.");
    }

    return;
  }

  // === Construir JSON dinámico ===
  String json = "{";
  json += "\"devEUI\":\"" + devEUI + "\",";
  json += "\"battery_voltage\":" + String(battery_voltage, 2) + ",";
  json += "\"battery_percentage\":" + String(battery_percentage) + ",";
  json += "\"panel_voltage\":" + String(panel_voltage, 2) + ",";
  json += "\"pulse_voltage\":" + String(pulse_voltage, 2) + ",";
  json += "\"pulse_time\":" + String(pulse_time) + ",";
  json += "\"battery_alliotec\":" + String(battery_alliotec, 2);
  json += "}";

  client.println("POST " + path + " HTTP/1.1");
  client.println("Host: " + host);
  client.println("User-Agent: PEON/1.0");
  client.println("Content-Type: application/json");
  client.println("Authorization: Bearer " + token);
  client.println("Connection: close");
  client.println("Content-Length: " + String(json.length()));
  client.println();  // Fin de headers
  client.print(json);  // Cuerpo

  SerialDebug.println("Respuesta:");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break; // fin de headers
  }
  while (client.available()) {
    SerialDebug.println(client.readStringUntil('\n'));
  }

  client.stop();
}*/

bool sendHttpPostJson(String tag,String host, String path, String token, String devEUI, String json) {
  if (WiFi.status() != WL_CONNECTED) {
    SerialDebug.println("❌ ERROR: Not connected to WiFi.");
    return false;
  }

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure());
  client->setInsecure();
  client->setBufferSizes(512, 512); 

  HTTPClient https;
  String url = "https://" + host + path;

  SerialDebug.print("============[ Attempting HTTPS connection for ");
  SerialDebug.print(tag);
  SerialDebug.println("]=============");

  SerialDebug.print("\t Host: "); SerialDebug.println(host);
  SerialDebug.print("\t Port: "); SerialDebug.println(443);
  SerialDebug.print("\t Path: "); SerialDebug.println(path);
  SerialDebug.print("\t Full URL: ");
  SerialDebug.println(url);

  if (!https.begin(*client, url)) {
    SerialDebug.println("\t ERROR: HTTPS connection failed (begin).");

    // Additional IP resolution diagnostics
    IPAddress ip;
    if (WiFi.hostByName(host.c_str(), ip)) {
      SerialDebug.print("\t Resolved IP: ");
      SerialDebug.println(ip); 
    } else {
      SerialDebug.println("\t ERROR: Failed to resolve DNS.");
    }

    SerialDebug.println("===================================");
    return false;
  }

  https.addHeader("Content-Type", "application/json");
  https.addHeader("Accept", "application/json");
  https.addHeader("User-Agent", "PEON/1.0");
  https.addHeader("Authorization", "Bearer " + token);

  int httpCode = https.POST(json);

  if(httpCode == 200) {
    SerialDebug.println("\t Server Response:");
    SerialDebug.print("\t ");
    SerialDebug.println(https.getString());
  } else {
    SerialDebug.println("\t ERROR: HTTP POST failed.");
    SerialDebug.printf("\t ❌ HTTP POST failed. code: %d  Error: %s\n\t Payload: %s",httpCode, https.errorToString(httpCode).c_str(), https.getString().c_str());
    //Print http headers for debug
    SerialDebug.println("\t HTTP Headers:");
    for (size_t i = 0; i < https.headers(); i++) {
      SerialDebug.print("\t ");
      SerialDebug.print(https.headerName(i));
      SerialDebug.print(": ");
      SerialDebug.println(https.header(i));
    }
  }
  
  SerialDebug.println("\n\t JSON Payload:");
  SerialDebug.print("\t ");
  SerialDebug.println(json);
  https.end();
  SerialDebug.println("===============================");
  return httpCode == 200;
}

bool sendDeviceDataByHttp(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec, uint8_t readSecuence, boolean isElectrifierTurnedOn) {

  String json = "{";
  json += "\"devEUI\":\"" + devEUI + "\",";
  json += "\"battery_voltage\":" + String(battery_voltage, 2) + ",";
  json += "\"battery_percentage\":" + String(battery_percentage) + ",";
  json += "\"panel_voltage\":" + String(panel_voltage, 2) + ",";
  json += "\"pulse_voltage\":" + String(pulse_voltage, 2) + ",";
  json += "\"pulse_time\":" + String(pulse_time) + ",";
  json += "\"battery_alliotec\":" + String(battery_alliotec, 2)+ ",";
  json += "\"read_secuence\":" + String(readSecuence) + ",";
  json += "\"electrifier_should_be_on\":" + String(isElectrifierTurnedOn ? "true" : "false");
  json += "}";

  return sendHttpPostJson("SendDeviceData", host, path, token, devEUI, json);
}


void ackCommandPost(String host, String path, String token, String devEUI, String command, boolean executionCommandDone, String response) {
  
  String json = "{";
  json += "\"devEUI\":\"" + devEUI + "\",";
  json += "\"action\":\"" + command + "\",";
  json += "\"resultExecutePeon\":" + String(executionCommandDone ? "true" : "false") + ",";
  json += "\"response\":\"" + response + "\",";
  json += "\"electrifier_should_be_on\":" + String(deviceStateService.isElectrifierTurnedOn() ? "true" : "false");
  json += "}" ;

  sendHttpPostJson("AckCommand", host, path + "/action", token, devEUI, json);
}

// GET request for commands, calls callback for each command if HTTP 200
// callback signature: void callback(const String& command, bool value, const String& ref)
bool getCommandsByHTTP(String host, String path, String token, String devEUI, void (*callback)(const String&)) {
  const int httpsPort = 443;

  SerialDebug.println("\n[🔌 Attempting GET connection...]");
  SerialDebug.print("[ℹ️] Host: "); SerialDebug.println(host);
  SerialDebug.print("[ℹ️] Port: "); SerialDebug.println(httpsPort);
  SerialDebug.print("[ℹ️] Path: "); SerialDebug.println(path+"/action");

  if (WiFi.status() != WL_CONNECTED) {
    SerialDebug.println("❌ ERROR: Not connected to WiFi.");
    return false;
  }

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure());
  client->setInsecure();
  client->setBufferSizes(512, 512);

  HTTPClient https;
  String url = "https://" + host + path+"/action?devEUI=" + devEUI;

  SerialDebug.print("[📡 Connecting to HTTPS to GET Commands URL] ");
  SerialDebug.println(url);

  if (!https.begin(*client, url)) {
    SerialDebug.println("❌ ERROR: HTTPS connection failed (begin).");
    return false;
  }

  https.addHeader("Authorization", "Bearer " + token);

  int httpCode = https.GET();

  if (httpCode == 200) {
    String payload = https.getString();
    SerialDebug.println("[📨 Server Response]:");
    SerialDebug.println(payload);

    // Parse JSON array
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      SerialDebug.print("❌ JSON parse error: ");
      SerialDebug.println(error.c_str());
      https.end();
      return false;
    }
    
    //validata is response is JSON object and contains key data
    if(!doc.is<JsonObject>() || !doc.containsKey("data")){
      SerialDebug.println("❌ JSON response is not an object or does not contain 'data' key.");
      https.end();
      return false;
    }

    //Response is JSON in key data I have the array of commands
    JsonArray commandsArray = doc["data"].as<JsonArray>();
    for (JsonObject obj : commandsArray) {
      String command = obj["action"] | "";
      if (callback) {
        callback(command);
      }
    }
    https.end();
    return true;
  } else {
    //Do serial debug error with enough information
    SerialDebug.printf("❌ HTTP GET Commands failed. code: %d  Error: %s\n Payload: %s" ,httpCode, https.errorToString(httpCode).c_str(), https.getString( ).c_str());
    //Print Get Params devEUI
    SerialDebug.print("[📡 GET Params] devEUI: ");
    SerialDebug.println(devEUI);
   
    https.end();
    return false;
  }
}

void sendLoRaWanCommandACK(uint8_t command, boolean executionCommandDone){
  uint8_t data[6];
  data[0] = LORAWAN_TX_ACK_VERSION; 
  data[1] = deviceStateService.isElectrifierTurnedOn() ? 0x01 : 0x00;
  data[2] = command;
  data[3] = executionCommandDone ? 0x01 : 0x00;
  uint16_t crc = calcCRC(data, 4);
  data[4] = crc & 0xFF;
  data[5] = (crc >> 8) & 0xFF;
  char hexPayload[13]; // 6 bytes * 2 hex chars + null terminator
  bytesToHexString(data, sizeof(data), hexPayload, sizeof(hexPayload));
  if(!lorawan.isJoined()){
      lorawan.join();
  }
  bool sentState = lorawan.send(LORAWAN_UPLOAD_LINK_ACK_FPORT, hexPayload, false);
  SerialDebug.print("LoRaWAN Command ACK send state: ");
  SerialDebug.println(sentState ? "SUCCESS" : "FAILURE");
  SerialDebug.print(hexPayload);
}

bool sendLoRaWan(float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec, uint8_t readSecuence){
  uint8_t data[12];
  uint16_t value;

  data[0] = 0x00; // Sensor port

  // Bytes 1-2: pulse_time (16 bits)
  value = (uint16_t)pulse_time;
  data[1] = (value >> 8) & 0xFF;
  data[2] = value & 0xFF;

  // Bytes 3-4: pulse_voltage (16 bits)
  value = (uint16_t)(pulse_voltage); // Mejor precisión si multiplicas por 100
  data[3] = (value >> 8) & 0xFF;
  data[4] = value & 0xFF;

  data[5] = (uint8_t)(panel_voltage * 10.0);
  data[6] = (uint8_t)(battery_voltage * 10.0);
  data[7] = (uint8_t)(battery_alliotec * 10.0);
  data[8] = deviceStateService.isElectrifierTurnedOn() ? 0x01 : 0x00;
  data[9] = readSecuence;

  uint16_t crc = calcCRC(data, 10);
  data[10] = crc & 0xFF;
  data[11] = (crc >> 8) & 0xFF;

  char hexPayload[25]; // 12 bytes * 2 hex chars + null terminator
  bytesToHexString(data, sizeof(data), hexPayload, sizeof(hexPayload));
  if(!lorawan.isJoined()){
      lorawan.join();
  }
  bool sentState = lorawan.send(LORAWAN_TELEMETRY_FPORT, hexPayload, false);
  SerialDebug.print("LoRaWAN send state: ");
  SerialDebug.println(sentState ? "SUCCESS" : "FAILURE");
  SerialDebug.print(hexPayload);
  return sentState;
}

void bytesToHexString(const uint8_t* data, size_t len, char* outHex, size_t outLen) {
  const char hexDigits[] = "0123456789ABCDEF";
  if (outLen < (len * 2 + 1)) return; // no hay espacio suficiente
  for (size_t i = 0; i < len; i++) {
    outHex[2*i]     = hexDigits[(data[i] >> 4) & 0x0F];
    outHex[2*i + 1] = hexDigits[data[i] & 0x0F];
  }
  outHex[len * 2] = '\0';
}

uint16_t calcCRC(const uint8_t *buf, uint8_t len) {
  uint16_t crc = 0xFFFF;

  for (uint8_t pos = 0; pos < len; pos++) {
    crc ^= buf[pos];

    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }

  return crc;
}


void requestCommandsOverHTTP(){
  getCommandsByHTTP(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), callbackForHttpCommands);
}


void callbackForHttpCommands(const String& command){
  SerialDebug.print("Received command via HTTP - Command: ");
  SerialDebug.println(command);

  if (command == "turnOnDevice") {
    if (!deviceStateService.isUltraEnergySavingMode()) {
      SerialDebug.println("Turning ON electrifier via HTTP command");
      const ElectrifierAckContext ack = {"turnOnDevice", LORAWAN_COMMANDS_TURN_ON_ELECTRIFIER, true, "ON"};
      turnOnElectrifier(true, &isElectrifierTurnedOn, &deviceStateService, &ack);
    } else {
      const String response = "Ultra Energy Saving Mode is active, cannot turn ON electrifier";
      SerialDebug.println(response);
      ackCommandOnActiveChannels("turnOnDevice", LORAWAN_COMMANDS_TURN_ON_ELECTRIFIER, false, response);
    }
    return;
  }

  if (command == "turnOffDevice") {
    SerialDebug.println("Turning OFF electrifier via HTTP command");
    const ElectrifierAckContext ack = {"turnOffDevice", LORAWAN_COMMANDS_TURN_OFF_ELECTRIFIER, true, "OFF"};
    turnOnElectrifier(false, &isElectrifierTurnedOn, &deviceStateService, &ack);
    return;
  }

  if (command == "readElectrifier") {
    SerialDebug.println("Reading electrifier via HTTP command");
    deviceStateService.updateStartAnalyzingProcess(true);
    ackCommandOnActiveChannels("readElectrifier", LORAWAN_COMMANDS_READ_ELECTRIFIER, true, "");
    return;
  }

  if (command == "checkElectrifierState") {
    SerialDebug.println("Checking electrifier state via HTTP command");
    ackCommandOnActiveChannels("checkElectrifierState", LORAWAN_COMMANDS_CHECK_ELECTRIFIER_ON_STATE, true, "");
    return;
  }

  SerialDebug.println("Unknown command received via HTTP");
  ackCommandOnActiveChannels(command, LORAWAN_COMMANDS_CHECK_ELECTRIFIER_ON_STATE, false, "Unknown command received via HTTP");
}


uint8_t hexToU8(const String& hex) {
  return (uint8_t) strtoul(hex.c_str(), nullptr, 16);
}



bool LoRaWanParseDownlink(const String& payloadHex, LoRaWanDownlinkContext* ctx) {
  //Documentation of the payload structure:
  // Byte 0: Version (1 byte)
  // Byte 1: Flags (1 byte)
  // Bytes 2-n: Commands (variable length)

  unsigned int index = 0;

  ctx->cmdCount = 0;

  if (payloadHex.length() < 4 || payloadHex.length() % 2 != 0) {
      SerialDebug.println("Invalid HEX payload");
    return false;
  }

  // ---- Version ----
  ctx->flags.version = hexToU8(payloadHex.substring(index, index + 2));
  index += 2;

  if (ctx->flags.version != 0x01) {
      SerialDebug.println("Unsupported version");
    return false;
  }

  // ---- Flags ----
  uint8_t rawFlags = hexToU8(payloadHex.substring(index, index + 2));
  index += 2;

  ctx->flags.atomicExecution = (rawFlags & 0x01) != 0;

    SerialDebug.print("Version: ");
    SerialDebug.print(ctx->flags.version);
    SerialDebug.print(" | Atomic: ");
    SerialDebug.println(ctx->flags.atomicExecution ? "YES" : "NO");

  // ---- Commands ----
  while ((index + 4) <= payloadHex.length() &&
         ctx->cmdCount < LORAWAN_MAX_CMDS) {

    LoRaWanRxCommand& c = ctx->cmds[ctx->cmdCount];
    c.valid = false;
    c.executed = false;
    c.success = false;

    c.cmd = hexToU8(payloadHex.substring(index, index + 2));
    index += 2;

    c.len = hexToU8(payloadHex.substring(index, index + 2));
    index += 2;

    if (c.len > LORAWAN_MAX_DATA_LEN ||
        index + c.len * 2 > payloadHex.length()) {
      SerialDebug.println("Invalid LEN, skipping command");
      index += c.len * 2;
      ctx->cmdCount++;
      continue;
    }

    for (uint8_t i = 0; i < c.len; i++) {
      c.data[i] = hexToU8(payloadHex.substring(index, index + 2));
      index += 2;
    }

    c.valid = true;
    ctx->cmdCount++;
  }

  // Log parsed commands
  SerialDebug.print("Parsed Commands Count: ");
  SerialDebug.println(ctx->cmdCount);
  for (uint8_t i = 0; i < ctx->cmdCount; i++) {
    const LoRaWanRxCommand& c = ctx->cmds[i];
    SerialDebug.print(" Command ");
    SerialDebug.print(i);
    SerialDebug.print(": CMD=0x");
    SerialDebug.print(c.cmd, HEX);
    SerialDebug.print(", LEN=");
    SerialDebug.print(c.len);
    SerialDebug.print(", DATA=");
    for (uint8_t j = 0; j < c.len; j++) {
      SerialDebug.print("0x");
      SerialDebug.print(c.data[j], HEX);
      if (j < c.len - 1) SerialDebug.print(" ");
    }
    SerialDebug.println(c.valid ? " [VALID]" : " [INVALID]");
  }
  return true;
}


bool LoRaWanValidateAllDownloadedCommands(const LoRaWanDownlinkContext* ctx) {

  for (uint8_t i = 0; i < ctx->cmdCount; i++) {
    const LoRaWanRxCommand& c = ctx->cmds[i];
    if (!c.valid) return false;

    switch (c.cmd) {
      case LORAWAN_COMMANDS_CHECK_ELECTRIFIER_ON_STATE:
      case LORAWAN_COMMANDS_TURN_ON_ELECTRIFIER:
      case LORAWAN_COMMANDS_TURN_OFF_ELECTRIFIER:
      case LORAWAN_COMMANDS_READ_ELECTRIFIER:
        if (c.len != 0) return false;
        break;
      default: return false;
    }
  }
  return true;
}


void LoRaWanExecuteDownloadedCommands(LoRaWanDownlinkContext* ctx) {

  if (ctx->flags.atomicExecution) {
      SerialDebug.println("Atomic execution enabled");

    if (!LoRaWanValidateAllDownloadedCommands(ctx)) {
      SerialDebug.println("Atomic validation failed, nothing executed");
      return;
    }
  }

  for (uint8_t i = 0; i < ctx->cmdCount; i++) {

    LoRaWanRxCommand& c = ctx->cmds[i];
    if (!c.valid) continue;

    c.executed = true;

    switch (c.cmd) {

      case LORAWAN_COMMANDS_TURN_ON_ELECTRIFIER: {
        const ElectrifierAckContext ack = {"turnOnDevice", LORAWAN_COMMANDS_TURN_ON_ELECTRIFIER, true, "ON"};
        turnOnElectrifier(true, &isElectrifierTurnedOn, &deviceStateService, &ack);
        c.success = true;
        break;
      }

      case LORAWAN_COMMANDS_TURN_OFF_ELECTRIFIER: {
        const ElectrifierAckContext ack = {"turnOffDevice", LORAWAN_COMMANDS_TURN_OFF_ELECTRIFIER, true, "OFF"};
        turnOnElectrifier(false, &isElectrifierTurnedOn, &deviceStateService, &ack);
        c.success = true;
        break;
      }

      case LORAWAN_COMMANDS_READ_ELECTRIFIER: {
        deviceStateService.updateStartAnalyzingProcess(true);
        c.success = true;
        ackCommandOnActiveChannels("readElectrifier", LORAWAN_COMMANDS_READ_ELECTRIFIER, true, "");
        break;
      }

      case LORAWAN_COMMANDS_CHECK_ELECTRIFIER_ON_STATE:
        c.success = true;
        ackCommandOnActiveChannels("checkElectrifierState", LORAWAN_COMMANDS_CHECK_ELECTRIFIER_ON_STATE, true, "");
        break;

      default:
        SerialDebug.println("Unknown command, cannot execute");
        SerialDebug.print(" CMD=0x");
        SerialDebug.println(c.cmd, HEX);
        c.success = false;
        ackCommandOnActiveChannels("unknown", c.cmd, false, "Unknown LoRaWAN command");
        break;
    }
  }
}

void processReceivedLoRaWanCommand(const String &lineIn){
  String line = lineIn;
  line.trim(); // MUY IMPORTANTE
  // Example line: +EVT:RX_C:-64:5:UNICAST:4:b076e8198c6454f77c56
  //Get each part RX_C | RSSI | SNR | TYPE | FPORT | PAYLOAD_HEX
  // Now suports  +EVT:RX_?
  if (!line.startsWith("+EVT:RX_")) {
    SerialDebug.println("Not RX_?");
    return;
  }

  // Campos tras el prefijo +EVT:RX_*:  (segundo ':' del mensaje)
  int colons = 0;
  int dataStart = -1;
  for (unsigned int i = 0; i < line.length(); i++) {
    if (line.charAt(i) == ':') {
      colons++;
      if (colons == 2) {
        dataStart = static_cast<int>(i) + 1;
        break;
      }
    }
  }
  if (dataStart < 0) {
    SerialDebug.println("Invalid RX_ format");
    return;
  }
  line = line.substring(dataStart);

  // Separar campos: RSSI | SNR | TYPE | FPORT | PAYLOAD_HEX
  int idx1 = line.indexOf(':');
  int idx2 = line.indexOf(':', idx1 + 1);
  int idx3 = line.indexOf(':', idx2 + 1);
  int idx4 = line.indexOf(':', idx3 + 1);

  if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1) {
    SerialDebug.println("Invalid RX_ field format");
    return;
  }

  int rssi = atoi(line.substring(0, idx1).c_str());
  int snr  = atoi(line.substring(idx1 + 1, idx2).c_str());
  String type = line.substring(idx2 + 1, idx3);
  int fport = atoi(line.substring(idx3 + 1, idx4).c_str());
  String payloadHex = line.substring(idx4 + 1);

  SerialDebug.print("Parsed RX_ - RSSI: "); SerialDebug.print(rssi);
  SerialDebug.print(", SNR: "); SerialDebug.print(snr);
  SerialDebug.print(", TYPE: "); SerialDebug.print(type);
  SerialDebug.print(", FPORT: "); SerialDebug.print(fport);
  SerialDebug.print(", PAYLOAD_HEX: "); SerialDebug.println(payloadHex);

  if(fport == LORAWAN_DOWNLOAD_LINK_COMMANDS_FPORT){
    SerialDebug.println("Processing Commands from LoRaWAN");
    LoRaWanDownlinkContext ctx;
    if(LoRaWanParseDownlink(payloadHex, &ctx)){
      LoRaWanExecuteDownloadedCommands(&ctx);
    } else {
      SerialDebug.println("Failed to parse downloaded commands");
    }
  }
}