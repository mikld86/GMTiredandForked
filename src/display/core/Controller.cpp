#include "Controller.h"
#include "../config.h"
#include "../drivers/LilyGo-T-RGB/LV_Helper.h"
#include "../plugins/HomekitPlugin.h"
#include "../plugins/WebUIPlugin.h"
#include "../plugins/mDNSPlugin.h"
#include "constants.h"
#include <SPIFFS.h>
#include <ctime>

Controller::Controller()
    : timer(nullptr), mode(MODE_BREW), currentTemp(0), grindActiveUntil(0), lastPing(0), lastProgress(0), lastAction(0),
      loaded(false), updating(false) {}

void Controller::setup() {
    mode = settings.getStartupMode();

    pluginManager = new PluginManager();
    ui = new DefaultUI(this, pluginManager);
    if (settings.isHomekit())
        pluginManager->registerPlugin(new HomekitPlugin(settings.getWifiSsid(), settings.getWifiPassword()));
    else
        pluginManager->registerPlugin(new mDNSPlugin());
    pluginManager->registerPlugin(new WebUIPlugin());
    pluginManager->setup(this);

    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
    }

    ui->init();
}

void Controller::onScreenReady() { screenReady = true; }

void Controller::onTargetChange(BrewTarget target) {
    settings.setVolumetricTarget(target == BrewTarget::VOLUMETRIC);
}

void Controller::connect() {
    if (initialized)
        return;
    lastPing = millis();
    pluginManager->trigger("controller:startup");

    setupBluetooth();
    setupWifi();

    updateLastAction();
    initialized = true;
}

void Controller::setupBluetooth() {
    clientController.initClient();
    clientController.registerTempReadCallback([this](const float temp) { onTempRead(temp); });
    pluginManager->trigger("controller:bluetooth:init");
}

void Controller::setupWifi() {
    if (settings.getWifiSsid() != "" && settings.getWifiPassword() != "") {
        WiFi.mode(WIFI_STA);
        WiFi.begin(settings.getWifiSsid(), settings.getWifiPassword());
        for (int attempts = 0; attempts < WIFI_CONNECT_ATTEMPTS; attempts++) {
            if (WiFi.status() == WL_CONNECTED) {
                break;
            }
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("");
            Serial.print("Connected to ");
            Serial.println(settings.getWifiSsid());
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            configTzTime(TIMEZONE, NTP_SERVER);
        } else {
            WiFi.disconnect(true, true);
            Serial.println("Timed out while connecting to WiFi");
        }
    }
    if (WiFi.status() != WL_CONNECTED) {
        isApConnection = true;
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_AP_SSID);
        Serial.println("Started in AP mode");
        Serial.print("Connect to:");
        Serial.println(WIFI_AP_SSID);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }

    pluginManager->on("ota:update:start", [this](Event const &) { this->updating = true; });
    pluginManager->on("ota:update:end", [this](Event const &) { this->updating = false; });

    pluginManager->trigger("controller:wifi:connect", "AP", isApConnection ? 1 : 0);
}

void Controller::loop() {
    ui->loop();
    pluginManager->loop();

    if (screenReady) {
        connect();
    }

    if (clientController.isReadyForConnection()) {
        clientController.connectToServer();
        pluginManager->trigger("controller:bluetooth:connect");
        if (!loaded) {
            loaded = true;
            if (settings.getStartupMode() == MODE_STANDBY)
                activateStandby();
        }
    }

    unsigned long now = millis();
    if (now - lastPing > PING_INTERVAL) {
        lastPing = now;
        clientController.sendPing();
    }

    if (now - lastProgress > PROGRESS_INTERVAL) {
        if (currentProcess != nullptr) {
            currentProcess->progress();
            if (!isActive()) {
                deactivate();
            }
        }
        clientController.sendTemperatureControl(getTargetTemp() + settings.getTemperatureOffset());
        clientController.sendPidSettings(settings.getPid());
        updateRelay();
        lastProgress = now;
    }

    if (grindActiveUntil != 0 && now > grindActiveUntil)
        deactivateGrind();
    if (mode != MODE_STANDBY && now > lastAction + STANDBY_TIMEOUT_MS)
        activateStandby();
}

bool Controller::isUpdating() const { return updating; }

int Controller::getTargetTemp() {
    switch (mode) {
    case MODE_BREW:
    case MODE_GRIND:
        return settings.getTargetBrewTemp();
    case MODE_STEAM:
        return settings.getTargetSteamTemp();
    case MODE_WATER:
        return settings.getTargetWaterTemp();
    default:
        return 0;
    }
}

void Controller::setTargetTemp(int temperature) {
    switch (mode) {
    case MODE_BREW:
    case MODE_GRIND:
        settings.setTargetBrewTemp(temperature);
        break;
    case MODE_STEAM:
        settings.setTargetSteamTemp(temperature);
        break;
    case MODE_WATER:
        settings.setTargetWaterTemp(temperature);
        break;
    default:;
    }
    updateLastAction();
    clientController.sendPidSettings(settings.getPid());
    clientController.sendTemperatureControl(getTargetTemp() + settings.getTemperatureOffset());
    pluginManager->trigger("boiler:targetTemperature:change", "value", getTargetTemp());
}

int Controller::getTargetDuration() const { return settings.getTargetDuration(); }

void Controller::setTargetDuration(int duration) {
    Event event = pluginManager->trigger("controller:targetDuration:change", "value", duration);
    settings.setTargetDuration(event.getInt("value"));
}

int Controller::getTargetGrindDuration() const { return settings.getTargetGrindDuration(); }

void Controller::setTargetGrindDuration(int duration) {
    Event event = pluginManager->trigger("controller:grindDuration:change", "value", duration);
    settings.setTargetGrindDuration(event.getInt("value"));
    updateLastAction();
}

void Controller::raiseTemp() {
    int temp = getTargetTemp();
    temp = max(MIN_TEMP, min(temp + 1, MAX_TEMP));
    setTargetTemp(temp);
}

void Controller::lowerTemp() {
    int temp = getTargetTemp();
    temp = max(MIN_TEMP, min(temp - 1, MAX_TEMP));
    setTargetTemp(temp);
}

void Controller::raiseBrewTarget() {
    if (settings.isVolumetricTarget()) {
        int newTarget = settings.getTargetVolume() + 1;
        if (newTarget > BREW_MAX_VOLUMETRIC) {
            newTarget = BREW_MAX_VOLUMETRIC;
        }
        settings.setVolumetricTarget(newTarget);
    } else {
        int newDuration = getTargetDuration() + 1000;
        if (newDuration > BREW_MAX_DURATION_MS) {
            newDuration = BREW_MIN_DURATION_MS;
        }
        setTargetDuration(newDuration);
    }
    updateLastAction();
}

void Controller::lowerBrewTarget() {
    if (settings.isVolumetricTarget()) {
        int newTarget = settings.getTargetVolume() - 1;
        if (newTarget < BREW_MIN_VOLUMETRIC) {
            newTarget = BREW_MIN_VOLUMETRIC;
        }
        settings.setVolumetricTarget(newTarget);
    } else {
        int newDuration = getTargetDuration() - 1000;
        if (newDuration < BREW_MIN_DURATION_MS) {
            newDuration = BREW_MIN_DURATION_MS;
        }
        setTargetDuration(newDuration);
    }
    updateLastAction();
}

void Controller::updateRelay() {
    clientController.sendPumpControl(isActive() ? currentProcess->getPumpValue() : 0);
    clientController.sendValveControl(isActive() && currentProcess->isRelayActive());
    clientController.sendAltControl(isGrindActive());
}

void Controller::activate() {
    if (isActive())
        return;
    switch (mode) {
    case MODE_BREW:
        if (settings.isVolumetricTarget()) {
            currentProcess = new BrewProcess(BrewTarget::VOLUMETRIC, settings.getInfusePumpTime(), settings.getInfuseBloomTime(),
                                             0, settings.getTargetVolume());
        } else {
            currentProcess = new BrewProcess(BrewTarget::TIME, settings.getInfusePumpTime(), settings.getInfuseBloomTime(),
                                             settings.getTargetDuration(), 0);
        }
        break;
    case MODE_STEAM:
        currentProcess = new SteamProcess();
        break;
    case MODE_WATER:
        currentProcess = new WaterProcess();
        break;
    default:;
    }
    updateRelay();
    updateLastAction();
    if (currentProcess->getType() == MODE_BREW) {
        pluginManager->trigger("controller:brew:start");
    }
}

void Controller::deactivate() {
    if (currentProcess == nullptr) {
        return;
    }
    if (currentProcess->getType() == MODE_BREW) {
        pluginManager->trigger("controller:brew:end");
    }
    delete (currentProcess);
    currentProcess = nullptr;
    updateRelay();
    updateLastAction();
}

void Controller::activateGrind() {
    pluginManager->trigger("controller:grind:start");
    if (isGrindActive())
        return;
    unsigned long duration = settings.getTargetGrindDuration();
    grindActiveUntil = millis() + duration;
    updateRelay();
    updateLastAction();
}

void Controller::deactivateGrind() {
    pluginManager->trigger("controller:grind:stop");
    grindActiveUntil = 0;
    updateRelay();
    updateLastAction();
}

void Controller::activateStandby() {
    setMode(MODE_STANDBY);
    deactivate();
}

void Controller::deactivateStandby() {
    deactivate();
    setMode(MODE_BREW);
}

bool Controller::isActive() const { return currentProcess != nullptr && currentProcess->isActive(); }

bool Controller::isGrindActive() const { return grindActiveUntil > millis(); }

int Controller::getMode() const { return mode; }

void Controller::setMode(int newMode) {
    Event modeEvent = pluginManager->trigger("controller:mode:change", "value", newMode);
    mode = modeEvent.getInt("value");

    updateLastAction();
    setTargetTemp(getTargetTemp());
}

void Controller::onTempRead(float temperature) {
    float temp = temperature - settings.getTemperatureOffset();
    Event event = pluginManager->trigger("boiler:currentTemperature:change", "value", temp);
    currentTemp = event.getFloat("value");
}

void Controller::updateLastAction() { lastAction = millis(); }

void Controller::onOTAUpdate() {
    activateStandby();
    updating = true;
}
