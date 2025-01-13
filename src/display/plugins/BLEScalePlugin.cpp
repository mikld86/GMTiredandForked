#include "BLEScalePlugin.h"
#include <display/core/Controller.h>
#include <scales/acaia.h>
#include <scales/bookoo.h>

void on_ble_measurement(float value) {
    BLEScales.onMeasurement(value);
}

BLEScalePlugin::BLEScalePlugin() : device(DiscoveredDevice(dummyDevice)) {}

void BLEScalePlugin::setup(Controller* controller, PluginManager* manager) {
    this->controller = controller;
    this->pluginRegistry = RemoteScalesPluginRegistry::getInstance();
    AcaiaScalesPlugin::apply();
    BookooScalesPlugin::apply();
    this->scanner = new RemoteScalesScanner();
    manager->on("controller:brew:start", [this](Event const &) {
       onBrewStart();
    });
    manager->on("controller:bluetooth:connect", [this](Event const &) {
        scanner->initializeAsyncScan();
    });
}

void BLEScalePlugin::loop() {
    if (doConnect && scale == nullptr) {
        establishConnection();
    }
    const unsigned long now = millis();
    if (now - lastUpdate > UPDATE_INTERVAL_MS) {
        lastUpdate = now;
        if (doConnect && scale != nullptr) {
            scale->update();
        }
    }
}

void BLEScalePlugin::connect(const std::string &uuid) {
    doConnect = true;
    this->uuid = uuid;
}

void BLEScalePlugin::scan() const {
    if (scale != nullptr && scale->isConnected()) {
        return;
    }
    scanner->initializeAsyncScan();
}

void BLEScalePlugin::disconnect() {
    if (scale != nullptr) {
        scale->disconnect();
        scale = nullptr;
    }
}

void BLEScalePlugin::onBrewStart() const {
    printf("Requesting scale tare");
    if (scale != nullptr && scale->isConnected()) {
        scale->tare();
        printf("Finished scale tare");
    }
}

void BLEScalePlugin::establishConnection() {
    scanner->stopAsyncScan();
    for (DiscoveredDevice d : scanner->getDiscoveredScales()) {
        if (d.getAddress().toString() == uuid) {
            this->device = DiscoveredDevice{d};
            scale = pluginRegistry->initialiseRemoteScales(device);
            if (!scale) {
                printf("Connection to device %s failed\n", device.getName().c_str());
                return;
            }
            if (!scale->connect()) {
                disconnect();
                return;
            }
            scale->setWeightUpdatedCallback(on_ble_measurement, false);
            break;
        }
    }
}

void BLEScalePlugin::onMeasurement(float value) const {
    if (controller != nullptr) {
        printf("Scale measurement: %.2f", value);
        controller->onVolumetricMeasurement(value);
    }
}

std::vector<DiscoveredDevice> BLEScalePlugin::getDiscoveredScales() const {
    return scanner->getDiscoveredScales();
}

DiscoveredDevice BLEScalePlugin::findDevice(const std::string &uuid) const {
    for (const DiscoveredDevice& d : scanner->getDiscoveredScales()) {
        if (d.getAddress().toString() == uuid) {
            return d;
        }
    }
    return nullptr;
}

BLEScalePlugin BLEScales;
