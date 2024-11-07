#include "NimBLEServerController.h"

NimBLEServerController::NimBLEServerController() :
    deviceConnected(false),
    tempControlChar(nullptr),
    pumpControlChar(nullptr),
    valveControlChar(nullptr),
    tempReadChar(nullptr),
    pingChar(nullptr),
    errorChar(nullptr),
    autotuneChar(nullptr),
    tempControlCallback(nullptr),
    pumpControlCallback(nullptr),
    valveControlCallback(nullptr),
    pidControlChar(nullptr),
    pidControlCallback(nullptr),
    pingCallback(nullptr),
    autotuneCallback(nullptr) {}

void NimBLEServerController::initServer() {
    NimBLEDevice::init("GPBLS");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Set to maximum power
    NimBLEDevice::setMTU(128);

    // Create BLE Server
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(this);  // Use this class as the callback handler

    // Create BLE Service
    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    // Temperature Control Characteristic (Client writes setpoint)
    tempControlChar = pService->createCharacteristic(
                        TEMP_CONTROL_CHAR_UUID,
                        NIMBLE_PROPERTY::WRITE
                      );
    tempControlChar->setCallbacks(this);  // Use this class as the callback handler

    // Pump Control Characteristic (Client writes pin state)
    pumpControlChar = pService->createCharacteristic(
                        PUMP_CONTROL_CHAR_UUID,
                        NIMBLE_PROPERTY::WRITE
                     );
    pumpControlChar->setCallbacks(this);  // Use this class as the callback handler

    // Valve Control Characteristic (Client writes pin state)
    valveControlChar = pService->createCharacteristic(
                        VALVE_CONTROL_CHAR_UUID,
                        NIMBLE_PROPERTY::WRITE
                     );
    valveControlChar->setCallbacks(this);  // Use this class as the callback handler

    // Temperature Read Characteristic (Server notifies client of temperature)
    tempReadChar = pService->createCharacteristic(
                     TEMP_READ_CHAR_UUID,
                     NIMBLE_PROPERTY::NOTIFY
                   );

    // Ping Characteristic (Client writes ping, Server reads)
    pingChar = pService->createCharacteristic(
                 PING_CHAR_UUID,
                 NIMBLE_PROPERTY::WRITE
               );
    pingChar->setCallbacks(this);  // Use this class as the callback handler

    // PID control Characteristic (Client writes PID settings, Server reads)
    pidControlChar = pService->createCharacteristic(
                 PID_CONTROL_CHAR_UUID,
                 NIMBLE_PROPERTY::WRITE
               );
    pidControlChar->setCallbacks(this);  // Use this class as the callback handler

    // Error Characteristic (Server writes error, Client reads)
    errorChar = pService->createCharacteristic(
                 ERROR_CHAR_UUID,
                 NIMBLE_PROPERTY::NOTIFY
               );

    // Ping Characteristic (Client writes autotune, Server reads)
    autotuneChar = pService->createCharacteristic(
                 AUTOTUNE_CHAR_UUID,
                 NIMBLE_PROPERTY::WRITE
               );
    autotuneChar->setCallbacks(this);  // Use this class as the callback handler

    pService->start();
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    NimBLEDevice::startAdvertising();
    Serial.println("BLE Server started, advertising...");
}

void NimBLEServerController::sendTemperature(float temperature) {
    if (deviceConnected) {
        // Send temperature notification to the client
        char tempStr[8];
        snprintf(tempStr, sizeof(tempStr), "%.2f", temperature);
        tempReadChar->setValue(tempStr);
        tempReadChar->notify();
    }
}

void NimBLEServerController::sendError(int errorCode) {
    if (deviceConnected) {
        // Send temperature notification to the client
        char errorStr[8];
        snprintf(errorStr, sizeof(errorStr), "%d", errorCode);
        errorChar->setValue(errorStr);
        errorChar->notify();
    }
}

void NimBLEServerController::registerTempControlCallback(temp_control_callback_t callback) {
    tempControlCallback = callback;
}

void NimBLEServerController::registerPumpControlCallback(pump_control_callback_t callback) {
    pumpControlCallback = callback;
}

void NimBLEServerController::registerValveControlCallback(pin_control_callback_t callback) {
    valveControlCallback = callback;
}

void NimBLEServerController::registerPingCallback(ping_callback_t callback) {
    pingCallback = callback;
}

void NimBLEServerController::registerAutotuneCallback(autotune_callback_t callback) {
    autotuneCallback = callback;
}

void NimBLEServerController::registerPidControlCallback(pid_control_callback_t callback) {
    pidControlCallback = callback;
}

// BLEServerCallbacks override
void NimBLEServerController::onConnect(NimBLEServer* pServer) {
    Serial.println("Client connected.");
    deviceConnected = true;
    pServer->stopAdvertising();
}

void NimBLEServerController::onDisconnect(NimBLEServer* pServer) {
    Serial.println("Client disconnected.");
    deviceConnected = false;
    pServer->startAdvertising();  // Restart advertising so clients can reconnect
}

String get_token(String &from, uint8_t index, char separator)
{
    uint16_t start = 0, idx = 0;
    uint8_t cur = 0;
    while (idx < from.length())
    {
        if (from.charAt(idx) == separator)
        {
            if (cur == index)
            {
                return from.substring(start, idx);
            }
            cur++;
            while (idx < from.length() - 1 && from.charAt(idx + 1) == separator) idx++;
            start = idx + 1;
        }
        idx++;
    }
    if ((cur == index) && (start < from.length()))
    {
        return from.substring(start, from.length());
    }
    return "";
}

// BLECharacteristicCallbacks override
void NimBLEServerController::onWrite(NimBLECharacteristic* pCharacteristic) {
    Serial.println("Write received!");

    if (pCharacteristic->getUUID().equals(NimBLEUUID(TEMP_CONTROL_CHAR_UUID))) {
        float setpoint = atof(pCharacteristic->getValue().c_str());
        Serial.printf("Received temperature setpoint: %.2f\n", setpoint);
        if (tempControlCallback != nullptr) {
            tempControlCallback(setpoint);
        }
    }
    else if (pCharacteristic->getUUID().equals(NimBLEUUID(PUMP_CONTROL_CHAR_UUID))) {
        float setpoint = atof(pCharacteristic->getValue().c_str());
        Serial.printf("Received pump control: %.2f\n", setpoint);
        if (pumpControlCallback != nullptr) {
            pumpControlCallback(setpoint);
        }
    }
    else if (pCharacteristic->getUUID().equals(NimBLEUUID(VALVE_CONTROL_CHAR_UUID))) {
        bool pinState = (pCharacteristic->getValue()[0] == '1');
        Serial.printf("Received valve control: %s\n", pinState ? "ON" : "OFF");
        if (valveControlCallback != nullptr) {
            valveControlCallback(pinState);
        }
    }
    else if (pCharacteristic->getUUID().equals(NimBLEUUID(PING_CHAR_UUID))) {
        Serial.printf("Received ping\n");
        if (pingCallback != nullptr) {
            pingCallback();
        }
    }
    else if (pCharacteristic->getUUID().equals(NimBLEUUID(AUTOTUNE_CHAR_UUID))) {
        Serial.printf("Received autotune\n");
        if (autotuneCallback != nullptr) {
            autotuneCallback();
        }
    }
    else if (pCharacteristic->getUUID().equals(NimBLEUUID(PID_CONTROL_CHAR_UUID))) {
        String pid = String(pCharacteristic->getValue().c_str());
        float Kp = get_token(pid, 0, ',').toFloat();
        float Ki = get_token(pid, 1, ',').toFloat();
        float Kd = get_token(pid, 2, ',').toFloat();
        Serial.printf("Received PID settings: %.2f, %.2f, %.2f\n", Kp, Ki, Kd);
        if (pidControlCallback != nullptr) {
            pidControlCallback(Kp, Ki, Kd);
        }
    }
}
