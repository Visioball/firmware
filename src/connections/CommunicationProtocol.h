#pragma once

#include <array>
#include <cstdint>

namespace CommunicationProtocol {
constexpr char SERVICE_UUID[] = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
constexpr char COMMAND_UUID[] = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
constexpr char BATTERY_UUID[] = "a1b2c3d4-1111-2222-3333-abcdefabcdef";
constexpr char STATUS_UUID[] = "a1b2c3d4-1111-2222-3333-abcdefabcdf0";
constexpr uint8_t VERSION = 1;
constexpr uint8_t LIGHT_ENABLED = 1 << 0;
constexpr uint8_t AUDIO_PLAYING = 1 << 1;
constexpr uint8_t BATTERY_AVAILABLE = 1 << 2;
constexpr uint8_t CHARGING_AVAILABLE = 1 << 3;
constexpr uint8_t CHARGING = 1 << 4;

struct DeviceStatus {
    bool lightEnabled = false;
    bool audioPlaying = false;
    bool batteryAvailable = false;
    bool chargingAvailable = false;
    bool charging = false;
    uint8_t lightIntensity = 0;
    uint8_t audioVolume = 0;
    uint8_t batteryLevel = 0;
};

inline std::array<uint8_t, 5> encode(const DeviceStatus& status) {
    const uint8_t flags = (status.lightEnabled ? LIGHT_ENABLED : 0) |
                          (status.audioPlaying ? AUDIO_PLAYING : 0) |
                          (status.batteryAvailable ? BATTERY_AVAILABLE : 0) |
                          (status.chargingAvailable ? CHARGING_AVAILABLE : 0) |
                          (status.chargingAvailable && status.charging ? CHARGING : 0);
    return {VERSION, flags, status.lightIntensity, status.audioVolume,
            status.batteryAvailable ? status.batteryLevel : uint8_t{0}};
}
}
