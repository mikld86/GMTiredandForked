#ifndef NIMBLESERVERCONTROLLER_H
#define NIMBLESERVERCONTROLLER_H

#include "NimBLEComm.h"

class NimBLEServerController : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
  public:
    NimBLEServerController();
    void initServer();
    void sendTemperature(float temperature);
    void sendError(int errorCode);
    void registerTempControlCallback(const temp_control_callback_t &callback);
    void registerPumpControlCallback(const pump_control_callback_t &callback);
    void registerValveControlCallback(const pin_control_callback_t &callback);
    void registerAltControlCallback(const pin_control_callback_t &callback);
    void registerPidControlCallback(const pid_control_callback_t &callback);
    void registerPingCallback(const ping_callback_t &callback);
    void registerAutotuneCallback(const autotune_callback_t &callback);

  private:
    bool deviceConnected = false;
    NimBLECharacteristic *tempControlChar;
    NimBLECharacteristic *pumpControlChar;
    NimBLECharacteristic *valveControlChar;
    NimBLECharacteristic *altControlChar;
    NimBLECharacteristic *tempReadChar;
    NimBLECharacteristic *pingChar;
    NimBLECharacteristic *pidControlChar;
    NimBLECharacteristic *errorChar;
    NimBLECharacteristic *autotuneChar;

    temp_control_callback_t tempControlCallback = nullptr;
    pump_control_callback_t pumpControlCallback = nullptr;
    pin_control_callback_t valveControlCallback = nullptr;
    pin_control_callback_t altControlCallback = nullptr;
    pid_control_callback_t pidControlCallback = nullptr;
    ping_callback_t pingCallback = nullptr;
    autotune_callback_t autotuneCallback = nullptr;

    // BLEServerCallbacks overrides
    void onConnect(NimBLEServer *pServer) override;
    void onDisconnect(NimBLEServer *pServer) override;

    // BLECharacteristicCallbacks overrides
    void onWrite(NimBLECharacteristic *pCharacteristic) override;
};

#endif // NIMBLESERVERCONTROLLER_H
