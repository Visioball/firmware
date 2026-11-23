#pragma once
#include <stddef.h>
#include "../light/LightSettings.h"

struct ConfigurationUpdate {
    bool hasVolume = false;
    float volume = 0.0f;
    bool hasAudioMode = false;
    int audioModeIndex = -1;
    bool hasLightStatus = false;
    bool hasLightIntensity = false;
    bool hasLightColor = false;
    LightSettings light;
};

enum class ConfigurationParseResult {
    Complete,
    Incomplete,
    Invalid
};

class ConfigurationParser {
public:
    ConfigurationParseResult parse(const char* json, size_t length,
                                   ConfigurationUpdate& update) const;
};