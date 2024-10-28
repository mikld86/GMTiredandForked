#include "homekit.h"


HomekitAccessory::HomekitAccessory(change_callback_t callback) : callback(nullptr), state(nullptr), targetState(nullptr), targetTemperature(nullptr), currentTemperature(nullptr), displayUnits(nullptr)  {
    this->callback=callback;
    state = new Characteristic::CurrentHeatingCoolingState();
    targetState = new Characteristic::TargetHeatingCoolingState();
    targetState->setValidValues(2, 0, 1);
    currentTemperature = new Characteristic::CurrentTemperature();
    currentTemperature->setRange(0, 160);
    targetTemperature = new Characteristic::TargetTemperature();
    targetTemperature->setRange(0, 160);
    displayUnits = new Characteristic::TemperatureDisplayUnits();
    displayUnits->setVal(0);
}

boolean HomekitAccessory::update() {
    if (targetState->getVal() != targetState->getNewVal()) {
        state->setVal(targetState->getNewVal());
        this->callback();
    }
    if (targetTemperature->getVal() != targetTemperature->getNewVal()) {
        this->callback();
    }
    return true;
}

boolean HomekitAccessory::getState() {
    return targetState->getVal() == 1;
}

void HomekitAccessory::setState(bool active) {
    this->targetState->setVal(active ? 1 : 0, true);
    this->state->setVal(active ? 1 : 0, true);
}

void HomekitAccessory::setCurrentTemperature(float temperatureValue) {
    currentTemperature->setVal(temperatureValue, true);
}

void HomekitAccessory::setTargetTemperature(float temperatureValue) {
    targetTemperature->setVal(temperatureValue, true);
}

float HomekitAccessory::getTargetTemperature() {
    return targetTemperature->getVal();
}

HomekitController::HomekitController() : accessory(nullptr), actionRequired(false) {
}

void HomekitController::initialize() {
    homeSpan.setHostNameSuffix("");
    homeSpan.setPortNum(HOMESPAN_PORT);
    homeSpan.begin(Category::Thermostats, "GaggiMate", "gaggia");
    homeSpan.setWifiCredentials(WIFI_SSID, WIFI_PASS);
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    accessory = new HomekitAccessory([this]() {
        this->actionRequired = true;
    });
    homeSpan.autoPoll();
}

void HomekitController::setCurrentTemperature(float temperatureValue) {
    if (accessory == nullptr) return;
    accessory->setCurrentTemperature(temperatureValue);
}

void HomekitController::setTargetTemperature(float temperatureValue) {
    if (accessory == nullptr) return;
    accessory->setTargetTemperature(temperatureValue);
}

void HomekitController::setState(bool active) {
    if (accessory == nullptr) return;
    accessory->setState(active);
}

bool HomekitController::hasAction() {
    return actionRequired;
}

bool HomekitController::getState() {
    return accessory->getState();
}

void HomekitController::clearAction() {
    actionRequired = false;
}

float HomekitController::getTargetTemperature() {
    if (accessory == nullptr) return 0;
    return accessory->getTargetTemperature();
}
