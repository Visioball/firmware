#pragma once
#include <Arduino.h>

void initWiFi();
bool isWiFiConnected();
String getWiFiIPAddress();
int8_t getWiFiRSSI();
void printWiFiStatus();

