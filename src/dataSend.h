#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

void startWiFi();

void sendData(float temperature, float humidity, int brightness);