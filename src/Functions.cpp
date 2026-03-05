
#include <Functions.h>
#include "Settings.h"
#include <ESP8266React.h>
#include <Adafruit_ADS1X15.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <DeviceStateService.h>

extern Adafruit_ADS1115 ads;

float getVoltage(uint8_t channel){
  int16_t val = ads.readADC_SingleEnded(channel);
  return ads.computeVolts(val);
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

float getSolarPannelVoltage(){
  return getVoltage(CHANNEL_PANEL)*SOLAR_FACTOR;
}

bool hasClientConnected(){
  return WiFi.softAPgetStationNum() > 0;
}


void turnOnElectrifier(boolean state, boolean* isTurnedOn, DeviceStateService* deviceStateService){
  *isTurnedOn = state;
  deviceStateService->updateElectrifierState(state);
  if(state){
    digitalWrite(PIN_TURN_ON_OFF_ELECTRIFIER, HIGH); // Turn on
    SerialDebug.println("Electrifier Turned ON");
  }else{    
    digitalWrite(PIN_TURN_ON_OFF_ELECTRIFIER, LOW); // Turn off
    SerialDebug.println("Electrifier Turned OFF");
  }
  notifyElectrifierStateChange(state);
  
}

void notifyElectrifierStateChange(boolean isTurnedOn){
  if(deviceSettingsService.isEnabled()){
    ackCommandPost(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), "electrifier_state_change", true, isTurnedOn ? "ON" : "OFF");
  }
  
  if(deviceLoRaWanSettingsService.isEnabled()){
    sendLoRaWanCommandACK( isTurnedOn ? LORAWAN_COMMANDS_TURN_ON_ELECTRIFIER : LORAWAN_COMMANDS_TURN_OFF_ELECTRIFIER, true);
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

void setupLoRaWan(){
  lorawan.setJoinMode(deviceLoRaWanSettingsService.shouldUseOtaa());
  lorawan.setClassMode(deviceLoRaWanSettingsService.getClassMode().charAt(0)); // 'A', 'B' o 'C'

  if(deviceLoRaWanSettingsService.shouldUseOtaa()){
    lorawan.setDevEUI(deviceLoRaWanSettingsService.getDevEUI().c_str());
    lorawan.setAppEUI(deviceLoRaWanSettingsService.getAppEUI().c_str());
    lorawan.setAppKey(deviceLoRaWanSettingsService.getAppKey().c_str());
  }else{
    lorawan.setDevAddr(deviceLoRaWanSettingsService.getDevAddress().c_str());
    lorawan.setAppSKey(deviceLoRaWanSettingsService.getAppsKey().c_str());
    lorawan.setNwkSKey(deviceLoRaWanSettingsService.getNetsKey().c_str());
  }
};

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

void sendHttpPostJson(String tag,String host, String path, String token, String devEUI, String json) {
  if (WiFi.status() != WL_CONNECTED) {
    SerialDebug.println("❌ ERROR: Not connected to WiFi.");
    return;
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
    return;
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
}

void sendDeviceDataByHttp(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec, boolean isElectrifierTurnedOn) {

  String json = "{";
  json += "\"devEUI\":\"" + devEUI + "\",";
  json += "\"battery_voltage\":" + String(battery_voltage, 2) + ",";
  json += "\"battery_percentage\":" + String(battery_percentage) + ",";
  json += "\"panel_voltage\":" + String(panel_voltage, 2) + ",";
  json += "\"pulse_voltage\":" + String(pulse_voltage, 2) + ",";
  json += "\"pulse_time\":" + String(pulse_time) + ",";
  json += "\"battery_alliotec\":" + String(battery_alliotec, 2)+ ",";
  json += "\"electrifier_should_be_on\":" + String(isElectrifierTurnedOn ? "true" : "false");
  json += "}";

  sendHttpPostJson("SendDeviceData", host, path, token, devEUI, json);
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

void sendLoRaWan(float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec){
  uint8_t data[11];
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

  uint16_t crc = calcCRC(data, 9);
  data[9] = crc & 0xFF;
  data[10] = (crc >> 8) & 0xFF;
  


   char hexPayload[23]; // 11 bytes * 2 hex chars + null terminator
  bytesToHexString(data, sizeof(data), hexPayload, sizeof(hexPayload));
  if(!lorawan.isJoined()){
      lorawan.join();
  }
  bool sentState = lorawan.send(12, hexPayload, false);
  SerialDebug.print("LoRaWAN send state: ");
  SerialDebug.println(sentState ? "SUCCESS" : "FAILURE");
  SerialDebug.print(hexPayload);

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
  SerialDebug.print(command);
  boolean executionCommandDone = false;
  String response = "";

  if(command == "turnOnDevice"){
    if(!deviceStateService.isUltraEnergySavingMode()){
      SerialDebug.println("Turning ON electrifier via HTTP command");
      turnOnElectrifier(true, &isElectrifierTurnedOn, &deviceStateService);
      executionCommandDone = true;
    }else{
      response = "Ultra Energy Saving Mode is active, cannot turn ON electrifier";
      SerialDebug.println("Cannot turn ON electrifier, Ultra Energy Saving Mode is active");
    }
  }else if(command == "turnOffDevice"){
    SerialDebug.println("Turning OFF electrifier via HTTP command");
    turnOnElectrifier(false, &isElectrifierTurnedOn, &deviceStateService);
    executionCommandDone = true;
  }else if(command == "checkElectrifierState"){
    SerialDebug.println("Checking electrifier state via HTTP command"); // Nothing to do, just ACK, ack will send the state
    executionCommandDone = true;
  }else{
    response = "Unknown command received via HTTP";
    SerialDebug.println("Unknown command received via HTTP");
  }

  ackCommandPost(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), command, executionCommandDone, response);
  
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

      case LORAWAN_COMMANDS_TURN_ON_ELECTRIFIER:
        turnOnElectrifier(true, &isElectrifierTurnedOn, &deviceStateService);
        c.success = true;
        break;

      case LORAWAN_COMMANDS_TURN_OFF_ELECTRIFIER:
        turnOnElectrifier(false, &isElectrifierTurnedOn, &deviceStateService);
        c.success = true;
        break;
      case LORAWAN_COMMANDS_CHECK_ELECTRIFIER_ON_STATE:
        c.success = true; //Dont ned to do anything, just ack 'cause ack will send the state
        break;
      default:
        SerialDebug.println("Unknown command, cannot execute");
        SerialDebug.print(" CMD=0x");
        SerialDebug.println(c.cmd, HEX);
        c.success = false;
        break;
    }

    sendLoRaWanCommandACK(c.cmd, c.success);
  }
}

void processReceivedLoRaWanCommand(String line){
  line.trim(); // MUY IMPORTANTE
  // Example line: +EVT:RX_C:-64:5:UNICAST:4:b076e8198c6454f77c56
  //Get each part RX_C | RSSI | SNR | TYPE | FPORT | PAYLOAD_HEX
  if (!line.startsWith("+EVT:RX_C:")) {
    SerialDebug.println("Not RX_C");
    return;
  }

  // Eliminar prefijo
  line.remove(0, 11); // strlen("+EVT:RX_C:")

  // Separar campos
  int idx1 = line.indexOf(':');
  int idx2 = line.indexOf(':', idx1 + 1);
  int idx3 = line.indexOf(':', idx2 + 1);
  int idx4 = line.indexOf(':', idx3 + 1);

  if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1) {
    SerialDebug.println("Invalid RX_C format");
    return;
  }

  int rssi = atoi(line.substring(0, idx1).c_str());
  int snr  = atoi(line.substring(idx1 + 1, idx2).c_str());
  String type = line.substring(idx2 + 1, idx3);
  int fport = atoi(line.substring(idx3 + 1, idx4).c_str());
  String payloadHex = line.substring(idx4 + 1);

  SerialDebug.print("Parsed RX_C - RSSI: "); SerialDebug.print(rssi);
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