#pragma once
#include <Arduino.h>
#include "../configuration/RuntimeConfiguration.h"
#include "CommunicationProtocol.h"

void bleInit(const char* deviceName, const CommunicationProtocol::DeviceStatus& initialStatus = {});
void bleManagerInit(const char* deviceName);
bool bleClientConnected();
bool receiveConfiguration(ConfigurationUpdate& update);
void publishDeviceStatus(const CommunicationProtocol::DeviceStatus& status);
bool publishAudioCatalog(const CommunicationProtocol::AudioMode* modes, size_t count);

void sendBattery(uint8_t percent);
void SendCharging(bool charging);

int GetCurrentSong();
bool IsPlayingBLE();
bool IsSleepingBLE();
float GetReceivedVolume();