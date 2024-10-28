//
// Created by Jochen Ullrich on 22.10.24.
//

#ifndef NIMBLESERVERCONTROLLER_H
#define NIMBLESERVERCONTROLLER_H

#include "NimBLEComm.h"

class NimBLEServerController : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
public:
    NimBLEServerController();
    void initServer();
    void sendTemperature(float temperature);
    void sendError(int errorCode);
    void registerTempControlCallback(temp_control_callback_t callback);
    void registerPumpControlCallback(pump_control_callback_t callback);
    void registerValveControlCallback(pin_control_callback_t callback);
    void registerPingCallback(ping_callback_t callback);
    void registerAutotuneCallback(autotune_callback_t callback);

private:
    bool deviceConnected;
    NimBLECharacteristic* tempControlChar;
    NimBLECharacteristic* pumpControlChar;
    NimBLECharacteristic* valveControlChar;
    NimBLECharacteristic* tempReadChar;
    NimBLECharacteristic* pingChar;
    NimBLECharacteristic* errorChar;
    NimBLECharacteristic* autotuneChar;

    temp_control_callback_t tempControlCallback;
    pump_control_callback_t pumpControlCallback;
    pin_control_callback_t valveControlCallback;
    ping_callback_t pingCallback;
    autotune_callback_t autotuneCallback;

    // BLEServerCallbacks overrides
    void onConnect(NimBLEServer* pServer) override;
    void onDisconnect(NimBLEServer* pServer) override;

    // BLECharacteristicCallbacks overrides
    void onWrite(NimBLECharacteristic* pCharacteristic) override;
};

#endif //NIMBLESERVERCONTROLLER_H
