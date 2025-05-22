#include <Functions.h>
#include <ESP8266React.h>
#include <Adafruit_ADS1X15.h>
#include "Settings.h"
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

float getBatteryVoltage(){
  return getVoltage(CHANNEL_BATTERY)*BATTERY_FACTOR;
}

float getSolarPannelVoltage(){
  return getVoltage(CHANNEL_PANEL)*SOLAR_FACTOR;
}

/*
void sendJsonPost(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setSNIHostname(host.c_str());

  Serial.println("[🔌 Intentando conexión]");
  Serial.print("Host: ");
  Serial.println(host);
  Serial.print("Puerto: 443\n");
  Serial.print("Path: ");
  Serial.println(path);

 if (!client.connect(host.c_str(), 443)) {
    Serial.println("❌ ERROR: No se pudo conectar al servidor HTTPS.");
    Serial.println("Posibles causas:");
    Serial.println("- ❗ Nombre del host mal escrito o incorrecto");
    Serial.println("- ❗ El servidor está fuera de línea o no responde por el puerto 443");
    Serial.println("- ❗ Problemas de DNS o red WiFi inestable");
    Serial.println("- ❗ El certificado SSL no es válido (aunque usamos setInsecure)");

    // Diagnóstico adicional: verificar IP
    IPAddress ip;
    if (WiFi.hostByName(host.c_str(), ip)) {
        Serial.print("🧭 Dirección IP resuelta: ");
        Serial.println(ip);
        Serial.println("➡️  El problema NO es DNS.");
    } else {
        Serial.println("❌ ERROR: No se pudo resolver el host DNS.");
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

  Serial.println("Respuesta:");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break; // fin de headers
  }
  while (client.available()) {
    Serial.println(client.readStringUntil('\n'));
  }

  client.stop();
}*/

void sendJsonPost(String host, String path, String token, String devEUI, float battery_voltage, float battery_percentage, float panel_voltage, float pulse_voltage, unsigned long pulse_time, float battery_alliotec) {
  const int httpsPort = 443;

  Serial.println("\n[🔌 Attempting connection...]");
  Serial.print("[ℹ️] Host: "); Serial.println(host);
  Serial.print("[ℹ️] Port: "); Serial.println(httpsPort);
  Serial.print("[ℹ️] Path: "); Serial.println(path);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ ERROR: Not connected to WiFi.");
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

  Serial.print("[📡 Connecting to HTTPS URL] ");
  Serial.println(url);

  if (!https.begin(*client, url)) {
    Serial.println("❌ ERROR: HTTPS connection failed (begin).");

    // Additional IP resolution diagnostics
    IPAddress ip;
    if (WiFi.hostByName(host.c_str(), ip)) {
      Serial.print("🧭 DNS resolved IP: ");
      Serial.println(ip);
      Serial.println("➡️  DNS resolution is OK.");
    } else {
      Serial.println("❌ ERROR: Failed to resolve DNS.");
    }

    return;
  }

  // Set headers and body
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + token);

  Serial.println("[📤 Sending POST request]");
  int httpCode = https.POST(json);

  if (httpCode > 0) {
    Serial.printf("[✅ HTTP Response Code]: %d\n", httpCode);
    String payload = https.getString();
    Serial.println("[📨 Server Response]:");
    Serial.println(payload);
  } else {
    Serial.printf("❌ HTTP POST failed. code: %d  Error: %s\n",httpCode, https.errorToString(httpCode).c_str());
  }

  https.end();
  Serial.println("[🔚 HTTPS session closed]");
}

