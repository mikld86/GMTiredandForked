#ifndef HOMEKIT_H
#define HOMEKIT_H

#include "config.h"
#include "HomeSpan.h"

#define HOMESPAN_PORT 8080
#define DEVICE_NAME "GaggiMate"

typedef std::function<void ()> change_callback_t;

class HomekitAccessory : public Service::Thermostat {
public:
    HomekitAccessory(change_callback_t callback);
    boolean getState();
    void setState(bool active);
    boolean update() override;
    void setCurrentTemperature(float temperatureValue);
    void setTargetTemperature(float temperatureValue);
    float getTargetTemperature();
private:
    change_callback_t callback;
    SpanCharacteristic *state;
    SpanCharacteristic *targetState;
    SpanCharacteristic *currentTemperature;
    SpanCharacteristic *targetTemperature;
    SpanCharacteristic *displayUnits;
};

class HomekitController {
public:
    HomekitController();
    void initialize(String wifiSsid, String wifiPassword);
    boolean hasAction();
    boolean getState();
    void clearAction();
    void setState(bool active);
    void setCurrentTemperature(float temperatureValue);
    void setTargetTemperature(float temperatureValue);
    float getTargetTemperature();

private:
    HomekitAccessory* accessory;
    bool actionRequired;
};

#endif
