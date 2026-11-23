#include "RuntimeConfiguration.h"
#include <ArduinoJson.h>
#include <cmath>

namespace {
bool readByte(JsonVariantConst value, uint8_t& output) {
    if (!value.is<int>()) return false;
    const int number = value.as<int>();
    if (number < 0 || number > 255) return false;
    output = static_cast<uint8_t>(number);
    return true;
}
}

ConfigurationParseResult ConfigurationParser::parse(const char* json, size_t length,
                                                    ConfigurationUpdate& update) const {
    JsonDocument document;
    const DeserializationError error = deserializeJson(document, json, length);
    if (error == DeserializationError::IncompleteInput) {
        return ConfigurationParseResult::Incomplete;
    }
    if (error || !document["configuration"].is<JsonObjectConst>()) {
        return ConfigurationParseResult::Invalid;
    }

    ConfigurationUpdate candidate;
    const JsonObjectConst configuration = document["configuration"];
    const JsonVariantConst audio = configuration["audio"];
    if (!audio.isUnbound()) {
        if (!audio.is<JsonObjectConst>()) return ConfigurationParseResult::Invalid;
        const JsonVariantConst volume = audio["volume"];
        if (!volume.isUnbound()) {
            if (!volume.is<float>()) return ConfigurationParseResult::Invalid;
            candidate.volume = volume.as<float>();
            if (!std::isfinite(candidate.volume) || candidate.volume < 0.0f ||
                candidate.volume > 100.0f) return ConfigurationParseResult::Invalid;
            candidate.hasVolume = true;
        }
    }

    const JsonVariantConst light = configuration["light"];
    if (!light.isUnbound()) {
        if (!light.is<JsonObjectConst>()) return ConfigurationParseResult::Invalid;
        if (!light["type"].isUnbound() && light["type"] != "static") {
            return ConfigurationParseResult::Invalid;
        }
        if (!light["status"].isUnbound()) {
            if (!light["status"].is<bool>()) return ConfigurationParseResult::Invalid;
            candidate.light.enabled = light["status"].as<bool>();
            candidate.hasLightStatus = true;
        }
        if (!light["intensity"].isUnbound()) {
            if (!readByte(light["intensity"], candidate.light.intensity)) {
                return ConfigurationParseResult::Invalid;
            }
            candidate.hasLightIntensity = true;
        }
        if (!light["color"].isUnbound()) {
            const JsonVariantConst color = light["color"];
            if (!color.is<JsonObjectConst>() ||
                !readByte(color["r"], candidate.light.red) ||
                !readByte(color["g"], candidate.light.green) ||
                !readByte(color["b"], candidate.light.blue)) {
                return ConfigurationParseResult::Invalid;
            }
            candidate.hasLightColor = true;
        }
    }

    if (!candidate.hasVolume && !candidate.hasLightStatus &&
        !candidate.hasLightIntensity && !candidate.hasLightColor) {
        return ConfigurationParseResult::Invalid;
    }
    update = candidate;
    return ConfigurationParseResult::Complete;
}