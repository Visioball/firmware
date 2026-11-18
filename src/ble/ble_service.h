#pragma once

#include <Arduino.h>

void initBle();

bool isBleClientConnected();

void updateBleStatus(const String& statusText);

void updateBleBattery(int batteryPercentage);

bool hasNewCommand();
String getLastCommand();