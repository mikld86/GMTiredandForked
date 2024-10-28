//
// Created by Jochen Ullrich on 22.10.24.
//

#ifndef NIMBLECLIENTCONTROLLER_H
#define NIMBLECLIENTCONTROLLER_H

#include "NimBLEComm.h"

class NimBLEClientController : public NimBLEAdvertisedDeviceCallbacks, NimBLEClientCallbacks {
public:
    NimBLEClientController();
    void initClient();
    bool connectToServer();
    void sendTemperatureControl(float setpoint);
    void sendPumpControl(float setpoint);
    void sendValveControl(bool pinState);
    void sendPing();
    void sendAutotune();
    bool isReadyForConnection();
    void scan();
    void registerTempReadCallback(temp_read_callback_t callback);
    void registerRemoteErrorCallback(remote_err_callback_t callback);

private:
    NimBLEClient* client;

    NimBLERemoteCharacteristic* tempControlChar;
    NimBLERemoteCharacteristic* pumpControlChar;
    NimBLERemoteCharacteristic* valveControlChar;
    NimBLERemoteCharacteristic* tempReadChar;
    NimBLERemoteCharacteristic* pingChar;
    NimBLERemoteCharacteristic* errorChar;
    NimBLERemoteCharacteristic* autotuneChar;
    NimBLEAdvertisedDevice* serverDevice;
    bool readyForConnection;

    temp_read_callback_t tempReadCallback;
    remote_err_callback_t remoteErrorCallback;

    // BLEAdvertisedDeviceCallbacks override
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override;

    // NimBLEClientCallbacks override
    void onDisconnect(NimBLEClient* pServer) override;

    // Notification callback
    void notifyCallback(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
};

#endif //NIMBLECLIENTCONTROLLER_H
