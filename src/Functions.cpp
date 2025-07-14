#include <Functions.h>
#include "Settings.h"
#include <ESP8266React.h>
#include <Adafruit_ADS1X15.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>

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

void sendJsonPost(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec) {
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

void sendLoRaWan(float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec){
  char command[64];
  uint8_t data[10];
  uint16_t value;

  data[0] = 0x00; // Sensor port

  // Bytes 1-2: pulse_time (16 bits)
  value = (uint16_t)pulse_time;
  data[1] = (value >> 8) & 0xFF;
  data[2] = value & 0xFF;

  // Bytes 3-4: pulse_voltage (16 bits)
  value = (uint16_t)(pulse_voltage * 100); // Mejor precisión si multiplicas por 100
  data[3] = (value >> 8) & 0xFF;
  data[4] = value & 0xFF;

  data[5] = (uint8_t)(panel_voltage * 10.0);
  data[6] = (uint8_t)(battery_voltage * 10.0);
  data[7] = (uint8_t)(battery_alliotec * 10.0);

  uint16_t crc = calcCRC(data, 8);
  data[8] = (crc >> 8) & 0xFF;
  data[9] = crc & 0xFF;


  buildLoRaCommand(data, sizeof(data), command, sizeof(command));
  SerialDebug.print(command);

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

void buildLoRaCommand(const uint8_t* data, size_t len, char* outBuffer, size_t outSize) {
  size_t pos = snprintf(outBuffer, outSize, "AT+SEND=1:");

  for (size_t i = 0; i < len && pos < outSize - 3; i++) {
    pos += snprintf(outBuffer + pos, outSize - pos, "%02X", data[i]);
  }

  // Agregar terminación \r\n si hay espacio
  snprintf(outBuffer + pos, outSize - pos, "\r\n");
}
