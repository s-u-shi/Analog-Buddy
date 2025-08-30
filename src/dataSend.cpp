#include "dataSend.h"
#include "private.h"

const int MAX_RETRY = 3;

void startWiFi() {
  WiFiManager manager;
  manager.setConfigPortalTimeout(180); //Timeout in 3 min

    for(int i = 0; i < MAX_RETRY; ++i) {
      if(manager.autoConnect("Analog Buddy", "AnalogBuddy2025")) {
        Serial.println("Connected to WiFi");
        return;
      }
      Serial.println("Connection Failed/Timeout, Please Retry");
      delay(2000);
    }

    Serial.println("Unable to connect to WiFi, device starting without connection to Cloud");
}

void sendData(float temperature, float humidity, int brightness) {
  //Create JSON payload
  ArduinoJson::JsonDocument doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["brightness"] = brightness;
  char buffer[256];
  serializeJson(doc, buffer, sizeof(buffer));

  //Send telemetry via HTTPS
  WiFiClientSecure client;
  client.setCACert(root_ca); //Set root CA certificate
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", SAS_TOKEN);
  int httpCode = http.POST(buffer);

  if (httpCode == 204) { //IoT Hub returns 204 (No Content) for successful telemetry
    Serial.println("Telemetry sent: " + String(buffer));
  }
  else {
    Serial.println("Failed to send telemetry. HTTP code: " + String(httpCode));
  }
  http.end();
}