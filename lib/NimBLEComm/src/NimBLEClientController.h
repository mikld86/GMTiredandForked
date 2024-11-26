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
    void sendAltControl(bool pinState);
    void sendPing();
    void sendAutotune();
    void sendPidSettings(const String &pid);
    bool isReadyForConnection() const;
    bool isConnected();
    void scan();
    void registerTempReadCallback(temp_read_callback_t &callback);
    void registerRemoteErrorCallback(remote_err_callback_t &callback);

  private:
    NimBLEClient *client;

    NimBLERemoteCharacteristic *tempControlChar;
    NimBLERemoteCharacteristic *pumpControlChar;
    NimBLERemoteCharacteristic *valveControlChar;
    NimBLERemoteCharacteristic *altControlChar;
    NimBLERemoteCharacteristic *tempReadChar;
    NimBLERemoteCharacteristic *pingChar;
    NimBLERemoteCharacteristic *pidControlChar;
    NimBLERemoteCharacteristic *errorChar;
    NimBLERemoteCharacteristic *autotuneChar;
    NimBLEAdvertisedDevice *serverDevice;
    bool readyForConnection = false;

    temp_read_callback_t tempReadCallback;
    remote_err_callback_t remoteErrorCallback;

    // BLEAdvertisedDeviceCallbacks override
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override;

    // NimBLEClientCallbacks override
    void onDisconnect(NimBLEClient *pServer) override;

    // Notification callback
    void notifyCallback(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) const;
};

#endif // NIMBLECLIENTCONTROLLER_H
