#pragma once
#include <Arduino.h>
#include "../configuration/RuntimeConfiguration.h"

void bleInit(const char* deviceName);
void bleManagerInit(const char* deviceName);
bool bleClientConnected();
bool receiveConfiguration(ConfigurationUpdate& update);

void sendBattery(uint8_t percent);
void SendCharging(bool charging);

int GetCurrentSong();
bool IsPlayingBLE();
bool IsSleepingBLE();
float GetReceivedVolume();