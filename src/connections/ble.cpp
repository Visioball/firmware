#include "ble.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdlib>

static BLEServer* bleServer = nullptr;
static BLECharacteristic* mainChar = nullptr;
static BLECharacteristic* batChar = nullptr;
static BLECharacteristic* statusChar = nullptr;
static std::array<uint8_t, 5> lastStatus{};
static float receivedVolume = 5.0f;

namespace {
constexpr size_t MAX_CONFIGURATION_BYTES = 1024;
constexpr uint32_t CONFIGURATION_TIMEOUT_MS = 5000;
QueueHandle_t configurationQueue = nullptr;
String configurationBuffer;
uint32_t lastConfigurationChunkAt = 0;
ConfigurationParser configurationParser;

bool queueConfiguration(const ConfigurationUpdate& update) {
    if (!configurationQueue || xQueueSend(configurationQueue, &update, 0) != pdTRUE) {
        Serial.println("[BLE] Configuration queue full or unavailable");
        return false;
    }
    if (update.hasVolume) receivedVolume = update.volume;
    return true;
}

void receiveConfigurationChunk(const String& chunk) {
    if (configurationBuffer.length() + chunk.length() > MAX_CONFIGURATION_BYTES) {
        configurationBuffer = "";
        Serial.println("[BLE] Configuration exceeds 1024 bytes");
        return;
    }
    configurationBuffer += chunk;
    lastConfigurationChunkAt = millis();

    ConfigurationUpdate update;
    const auto result = configurationParser.parse(configurationBuffer.c_str(),
                                                  configurationBuffer.length(), update);
    if (result == ConfigurationParseResult::Incomplete) return;
    configurationBuffer = "";
    if (result == ConfigurationParseResult::Invalid) {
        Serial.println("[BLE] Invalid configuration");
        return;
    }
    if (queueConfiguration(update)) {
        Serial.println("[BLE] Configuration received");
    }
}
}

static int song = 0;
static const int songAmount = 5;

std::atomic<bool> isPlayingBLE{false};
bool isSleepingBLE = false;

int GetCurrentSong() {
    return song;
}

bool IsPlayingBLE() {
    return isPlayingBLE.load();
}

float GetReceivedVolume() {
    return receivedVolume;
}

bool receiveConfiguration(ConfigurationUpdate& update) {
    return configurationQueue && xQueueReceive(configurationQueue, &update, 0) == pdTRUE;
}

bool IsSleepingBLE() {
    return isSleepingBLE;
}

void sendBattery(uint8_t percent) {
    if (!batChar) return;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "BAT%%:%u", percent);
    batChar->setValue((uint8_t*)buffer, strlen(buffer));
    batChar->notify();
}

void SendCharging(bool charging) {
    if (!batChar) return;

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "CHARGING:%s", charging ? "TRUE" : "FALSE");
    batChar->setValue((uint8_t*)buffer, strlen(buffer));
    batChar->notify();
}

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        Serial.println("[BLE] Client connected");
    }

    void onDisconnect(BLEServer* server) override {
        configurationBuffer = "";
        Serial.println("[BLE] Client disconnected");
        delay(100);
        server->getAdvertising()->start();
        Serial.println("[BLE] Advertising restarted");
    }
};

class CharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        String value = characteristic->getValue();

        if (configurationBuffer.length() &&
            millis() - lastConfigurationChunkAt > CONFIGURATION_TIMEOUT_MS) {
            configurationBuffer = "";
            Serial.println("[BLE] Incomplete configuration timed out");
        }
        if (configurationBuffer.length()) {
            receiveConfigurationChunk(value);
            return;
        }
        size_t firstCharacter = 0;
        while (firstCharacter < value.length() &&
               std::isspace(static_cast<unsigned char>(value[firstCharacter]))) {
            ++firstCharacter;
        }
        if (firstCharacter < value.length() && value[firstCharacter] == '{') {
            receiveConfigurationChunk(value);
            return;
        }
        value.trim();

        if (value.startsWith("MODE:")) {
            const char* number = value.c_str() + 5;
            char* end = nullptr;
            long modeIndex = std::strtol(number, &end, 10);
            if (end == number || *end != '\0' || modeIndex < 0 || modeIndex > 17) {
                Serial.println("[BLE] Invalid audio mode");
                return;
            }
            ConfigurationUpdate update;
            update.hasAudioMode = true;
            update.audioModeIndex = static_cast<int>(modeIndex);
            queueConfiguration(update);
        } else if (value.startsWith("VOL:")) {
            const char* number = value.c_str() + 4;
            char* end = nullptr;
            ConfigurationUpdate update;
            update.volume = std::strtof(number, &end);
            if (end == number || *end != '\0' || !std::isfinite(update.volume) ||
                update.volume < 0.0f || update.volume > 100.0f) {
                Serial.println("[BLE] Invalid volume");
                return;
            }
            update.hasVolume = true;
            queueConfiguration(update);
        } else if (value == "SLEEP") {
            if (isSleepingBLE) return;
            isSleepingBLE = true;
            Serial.println("[CMD] SLEEP -> entering sleep mode");
        } else if (value == "WAKE") {
            if (!isSleepingBLE) return;
            isSleepingBLE = false;
            Serial.println("[CMD] WAKE -> waking up");
        } else if (value == "PAUSE") {
            if (!isPlayingBLE.exchange(false)) return;
            Serial.println("[CMD] PAUSE -> paused");
        } else if (value == "PLAY") {
            if (isPlayingBLE.exchange(true)) return;
            Serial.println("[CMD] PLAY -> playing");
        } else if (value == "NEXT") {
            song = (song + 1) % songAmount;
            Serial.printf("[CMD] NEXT -> song %d\n", song);
        } else if (value == "PREV") {
            song = (song - 1 + songAmount) % songAmount;
            Serial.printf("[CMD] PREV -> song %d\n", song);
        } else if (value == "SONG1") {
            song = 0;
            Serial.println("[CMD] SONG1");
        } else if (value == "SONG2") {
            song = 1;
            Serial.println("[CMD] SONG2");
        } else if (value == "SONG3") {
            song = 2;
            Serial.println("[CMD] SONG3");
        } else if (value == "SONG4") {
            song = 3;
            Serial.println("[CMD] SONG4");
        } else if (value == "SONG5") {
            song = 4;
            Serial.println("[CMD] SONG5");
        } else {
            Serial.printf("[CMD] Unknown: %s\n", value.c_str());
        }
    }
};

void publishDeviceStatus(const CommunicationProtocol::DeviceStatus& status) {
    if (!statusChar) return;
    const auto encoded = CommunicationProtocol::encode(status);
    if (encoded == lastStatus) return;
    lastStatus = encoded;
    statusChar->setValue(lastStatus.data(), lastStatus.size());
    if (bleClientConnected()) statusChar->notify();
}

void bleInit(const char* name, const CommunicationProtocol::DeviceStatus& initialStatus) {
    configurationQueue = xQueueCreate(8, sizeof(ConfigurationUpdate));
    if (!configurationQueue) {
        Serial.println("[BLE] Cannot allocate configuration queue");
        return;
    }
    configurationBuffer.reserve(MAX_CONFIGURATION_BYTES);
    BLEDevice::init(name);
    BLEDevice::setMTU(517);
    BLEDevice::setPower(ESP_PWR_LVL_P9);

    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new ServerCallbacks());

    BLEService* service = bleServer->createService(CommunicationProtocol::SERVICE_UUID);
    mainChar = service->createCharacteristic(
        CommunicationProtocol::COMMAND_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    batChar = service->createCharacteristic(
        CommunicationProtocol::BATTERY_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    statusChar = service->createCharacteristic(
        CommunicationProtocol::STATUS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    publishDeviceStatus(initialStatus);

    mainChar->setCallbacks(new CharCallbacks());
    mainChar->setValue("ACTIVE");
    service->start();

    BLEAdvertising* advertising = bleServer->getAdvertising();
    BLEAdvertisementData advertisementData;
    advertisementData.setFlags(0x06);
    advertisementData.setCompleteServices(BLEUUID(CommunicationProtocol::SERVICE_UUID));
    advertising->setAdvertisementData(advertisementData);

    BLEAdvertisementData scanResponseData;
    scanResponseData.setName(String(name));
    advertising->setScanResponseData(scanResponseData);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    advertising->start();

    Serial.printf("[BLE] Advertising as '%s' with service %s\n", name, CommunicationProtocol::SERVICE_UUID);
}

void bleManagerInit(const char* deviceName) {
    bleInit(deviceName);
}

bool bleClientConnected() {
    return bleServer && bleServer->getConnectedCount() > 0;
}