#include "ble_service.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "../config/app_config.h"

static BLEServer*         bleServer = nullptr;
static BLEService*        bleServiceInstance = nullptr;
static BLECharacteristic* bleStatusChar = nullptr;
static BLECharacteristic* bleCommandChar = nullptr;
static bool               bleDeviceConnected = false;

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        bleDeviceConnected = true;
        Serial.println("[BLE] Client connected.");
    }

    void onDisconnect(BLEServer* pServer) override {
        bleDeviceConnected = false;
        Serial.println("[BLE] Client disconnected. Restarting advertising...");
        BLEDevice::startAdvertising();
    }
};

void initBle() {
    Serial.printf("[BLE] Initializing BLE Device: %s\n", AppConfig::Ble::DEVICE_NAME);

    BLEDevice::init(AppConfig::Ble::DEVICE_NAME);
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new ServerCallbacks());

    bleServiceInstance = bleServer->createService(AppConfig::Ble::SERVICE_UUID);

    bleStatusChar = bleServiceInstance->createCharacteristic(
        AppConfig::Ble::CHAR_STATUS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    bleStatusChar->addDescriptor(new BLE2902());
    bleStatusChar->setValue("Ready");

    bleCommandChar = bleServiceInstance->createCharacteristic(
        AppConfig::Ble::CHAR_COMMAND_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );

    bleServiceInstance->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(AppConfig::Ble::SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("[BLE] Advertising started. Waiting for client connection...");
}

bool isBleClientConnected() {
    return bleDeviceConnected;
}

void updateBleStatus(const String& statusText) {
    if (bleStatusChar) {
        bleStatusChar->setValue(statusText.c_str());
        if (bleDeviceConnected) {
            bleStatusChar->notify();
        }
    }
}

