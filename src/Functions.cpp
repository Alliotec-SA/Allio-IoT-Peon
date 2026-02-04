
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

void sendJsonPost(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec, boolean isElectrifierTurnedOn) {
  const int httpsPort = 443;

  SerialDebug.println("\n[🔌 Attempting connection...]");
  SerialDebug.print("[ℹ️] Host: "); SerialDebug.println(host);
  SerialDebug.print("[ℹ️] Port: "); SerialDebug.println(httpsPort);
  SerialDebug.print("[ℹ️] Path: "); SerialDebug.println(path);

  if (WiFi.status() != WL_CONNECTED) {
    SerialDebug.println("❌ ERROR: Not connected to WiFi.");
    return;
  }

  // Build JSON payload
  String json = "{";
  json += "\"devEUI\":\"" + devEUI + "\",";
  json += "\"battery_voltage\":" + String(battery_voltage, 2) + ",";
  json += "\"battery_percentage\":" + String(battery_percentage) + ",";
  json += "\"panel_voltage\":" + String(panel_voltage, 2) + ",";
  json += "\"pulse_voltage\":" + String(pulse_voltage, 2) + ",";
  json += "\"pulse_time\":" + String(pulse_time) + ",";
  json += "\"battery_alliotec\":" + String(battery_alliotec, 2);
  json += "\"electrifier_should_be_on\":" + String(isElectrifierTurnedOn ? "true" : "false");
  json += "}";

  // Create secure client and disable SSL validation
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure());
  client->setInsecure();
  client->setBufferSizes(512, 512); 

  HTTPClient https;
  String url = "https://" + host + path;

  SerialDebug.print("[📡 Connecting to HTTPS URL] ");
  SerialDebug.println(url);

  if (!https.begin(*client, url)) {
    SerialDebug.println("❌ ERROR: HTTPS connection failed (begin).");

    // Additional IP resolution diagnostics
    IPAddress ip;
    if (WiFi.hostByName(host.c_str(), ip)) {
      SerialDebug.print("🧭 DNS resolved IP: ");
      SerialDebug.println(ip);
      SerialDebug.println("➡️  DNS resolution is OK.");
    } else {
      SerialDebug.println("❌ ERROR: Failed to resolve DNS.");
    }

    return;
  }

  // Set headers and body
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + token);

  SerialDebug.println("[📤 Sending POST request]");
  int httpCode = https.POST(json);

  if (httpCode > 0) {
    SerialDebug.printf("[✅ HTTP Response Code]: %d\n", httpCode);
    String payload = https.getString();
    SerialDebug.println("[📨 Server Response]:");
    SerialDebug.println(payload);
  } else {
    SerialDebug.printf("❌ HTTP POST failed. code: %d  Error: %s\n",httpCode, https.errorToString(httpCode).c_str());
  }

  https.end();
  SerialDebug.println("[🔚 HTTPS session closed]");
}


void ackCommandPost(String host, String path, String token, String devEUI, String command, boolean executionCommandDone, String response) {
  const int httpsPort = 443;

  SerialDebug.println("\n[🔌 Attempting connection...]");
  SerialDebug.print("[ℹ️] Host: "); SerialDebug.println(host);
  SerialDebug.print("[ℹ️] Port: "); SerialDebug.println(httpsPort);
  SerialDebug.print("[ℹ️] Path: "); SerialDebug.println(path);

  if (WiFi.status() != WL_CONNECTED) {
    SerialDebug.println("❌ ERROR: Not connected to WiFi.");
    return;
  }

  // Build JSON payload
  String json = "{";
  json += "\"devEUI\":\"" + devEUI + "\",";
  json += "\"action\":\"" + command + "\",";
  json += "\"resultExecutePeon\":" + String(executionCommandDone ? "true" : "false") + ",";
  json += "\"response\":\"" + response + "\",";
  json += "\"electrifier_should_be_on\":" + String(deviceStateService.isElectrifierTurnedOn() ? "true" : "false");
  json += "}" ;

  // Create secure client and disable SSL validation
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure());
  client->setInsecure();
  client->setBufferSizes(512, 512); 

  HTTPClient https;
  String url = "https://" + host + path + "/action" ;

  SerialDebug.print("[📡 Connecting to HTTPS URL] ");
  SerialDebug.println(url);

  if (!https.begin(*client, url)) {
    SerialDebug.println("❌ ERROR: HTTPS connection failed (begin).");

    // Additional IP resolution diagnostics
    IPAddress ip;
    if (WiFi.hostByName(host.c_str(), ip)) {
      SerialDebug.print("🧭 DNS resolved IP: ");
      SerialDebug.println(ip);
      SerialDebug.println("➡️  DNS resolution is OK.");
    } else {
      SerialDebug.println("❌ ERROR: Failed to resolve DNS.");
    }

    return;
  }

  // Set headers and body
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + token);

  SerialDebug.println("[📤 Sending POST request]");
  int httpCode = https.POST(json);

  if (httpCode == 200) {
    SerialDebug.printf("[✅ HTTP Response Code]: %d\n", httpCode);
    String payload = https.getString();
    SerialDebug.println("[📨 Server Response]:");
    SerialDebug.println(payload);
  } else {
    //Print error with enough information also include error string, payload response if exist and sended data
    SerialDebug.printf("❌ HTTP POST failed. code: %d  Error: %s\n Payload: %s",httpCode, https.errorToString(httpCode).c_str(), https.getString().c_str());
    //Print sent data
    SerialDebug.println("[📤 Sent JSON Payload]:");
    SerialDebug.println(json);
  }

  https.end();
  SerialDebug.println("[🔚 HTTPS session closed]");
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

  SerialDebug.print("[📡 Connecting to HTTPS URL] ");
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
    SerialDebug.printf("❌ HTTP GET failed. code: %d  Error: %s\n Payload: %s" ,httpCode, https.errorToString(httpCode).c_str(), https.getString( ).c_str());
   
    https.end();
    return false;
  }
}

void sendLoRaWan(float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec){
  uint8_t data[10];
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

  uint16_t crc = calcCRC(data, 8);
  data[8] = crc & 0xFF;
  data[9] = (crc >> 8) & 0xFF;
  


   char hexPayload[21]; // 10 bytes * 2 hex chars + null terminator
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
  }else{
    response = "Unknown command received via HTTP";
    SerialDebug.println("Unknown command received via HTTP");
  }

  ackCommandPost(deviceSettingsService.getServer(), deviceSettingsService.getPath(), deviceSettingsService.getToken(), deviceSettingsService.getDevEUI(), command, executionCommandDone, response);
  
}

