#ifndef HOMEKITPLUGIN_H
#define HOMEKITPLUGIN_H
#include "../config.h"
#include "../Plugin.h"
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

class HomekitPlugin : public Plugin {
  public:
    HomekitPlugin(String wifiSsid, String wifiPassword);
    void setup(Controller *controller, PluginManager *pluginManager) override;
    void loop() override;

    boolean hasAction() const;
    void clearAction();

private:
    String wifiSsid;
    String wifiPassword;
    HomekitAccessory* accessory;
    bool actionRequired;
    Controller* controller;
};



#endif //HOMEKITPLUGIN_H
