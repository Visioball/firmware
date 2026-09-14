#include "wifi_service.h"
#include <WiFi.h>
#include "../config/app_config.h"

static void startAccessPoint() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AppConfig::WiFi::AP_SSID, AppConfig::WiFi::AP_PASSWORD);
    Serial.printf("[WiFi] AP Mode Active. SSID: %s | IP: %s\n",
                  AppConfig::WiFi::AP_SSID, WiFi.softAPIP().toString().c_str());
}

static bool connectStation() {
    Serial.printf("[WiFi] Connecting to SSID: %s\n", AppConfig::WiFi::SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(AppConfig::WiFi::SSID, AppConfig::WiFi::PASSWORD);

    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < AppConfig::WiFi::CONNECT_TIMEOUT_MS)) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected! IP: %s | RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    }

    return false;
}

void initWiFi() {
    Serial.println("\n[WiFi] Initializing WiFi Service...");

    if (strcmp(AppConfig::WiFi::SSID, "Your_WiFi_SSID") == 0) {
        Serial.println("[WiFi] Default credentials detected. Starting Fallback AP mode...");
        startAccessPoint();
        return;
    }

    if (!connectStation()) {
        Serial.println("[WiFi] Station connection failed. Starting Fallback AP mode...");
        startAccessPoint();
    }
}

bool isWiFiConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

String getWiFiIPAddress() {
    if (isWiFiConnected()) {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

int8_t getWiFiRSSI() {
    return WiFi.RSSI();
}

void printWiFiStatus() {
    if (isWiFiConnected()) {
        Serial.printf("[WiFi Status] STA Connected | IP: %s | RSSI: %d dBm\n",
                      getWiFiIPAddress().c_str(), getWiFiRSSI());
    } else {
        Serial.printf("[WiFi Status] AP Active | IP: %s\n", getWiFiIPAddress().c_str());
    }
}

