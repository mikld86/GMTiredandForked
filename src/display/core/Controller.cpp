#include "Controller.h"
#include "ArduinoJson.h"
#include <SPIFFS.h>
#include <ctime>
#include <display/config.h>
#include <display/core/constants.h>
#include <display/core/zones.h>
#include <display/plugins/BLEScalePlugin.h>
#include <display/plugins/BoilerFillPlugin.h>
#include <display/plugins/HomekitPlugin.h>
#include <display/plugins/MQTTPlugin.h>
#include <display/plugins/SmartGrindPlugin.h>
#include <display/plugins/WebUIPlugin.h>
#include <display/plugins/mDNSPlugin.h>

void Controller::setup() {
    mode = settings.getStartupMode();

    pluginManager = new PluginManager();
    ui = new DefaultUI(this, pluginManager);
    if (settings.isHomekit())
        pluginManager->registerPlugin(new HomekitPlugin(settings.getWifiSsid(), settings.getWifiPassword()));
    else
        pluginManager->registerPlugin(new mDNSPlugin());
    if (settings.isBoilerFillActive()) {
        pluginManager->registerPlugin(new BoilerFillPlugin());
    }
    if (settings.isSmartGrindActive()) {
        pluginManager->registerPlugin(new SmartGrindPlugin());
    }
    if (settings.isHomeAssistant()) {
        pluginManager->registerPlugin(new MQTTPlugin());
    }
    pluginManager->registerPlugin(new WebUIPlugin());
    pluginManager->registerPlugin(&BLEScales);
    pluginManager->setup(this);

    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
    }

    ui->init();
}

void Controller::onScreenReady() { screenReady = true; }

void Controller::onTargetChange(ProcessTarget target) { settings.setVolumetricTarget(target == ProcessTarget::VOLUMETRIC); }

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
    clientController.registerBrewBtnCallback([this](const int brewButtonStatus) { handleBrewButton(brewButtonStatus); });
    clientController.registerSteamBtnCallback([this](const int steamButtonStatus) { handleSteamButton(steamButtonStatus); });
    clientController.registerRemoteErrorCallback([this](const int error) {
        if (error != ERROR_CODE_TIMEOUT && error != this->error) {
            this->error = error;
            deactivate();
            setMode(MODE_STANDBY);
            pluginManager->trigger("controller:error");
        }
        ESP_LOGE("Controller", "Received error %d", error);
    });
    clientController.registerAutotuneResultCallback([this](const float Kp, const float Ki, const float Kd) {
        ESP_LOGI("Controller", "Received new autotune values: %.3f, %.3f, %.3f", Kp, Ki, Kd);
        char pid[30];
        snprintf(pid, sizeof(pid), "%.3f,%.3f,%.3f", Kp, Ki, Kd);
        settings.setPid(String(pid));
        pluginManager->trigger("controller:autotune:result");
        autotuning = false;
    });
    pluginManager->trigger("controller:bluetooth:init");
}

void Controller::setupInfos() {
    const std::string info = clientController.readInfo();
    printf("System info: %s\n", info.c_str());
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, info);
    if (err) {
        printf("Error deserializing JSON: %s\n", err.c_str());
        systemInfo = SystemInfo{
            .hardware = "GaggiMate Standard 1.x", .version = "v1.0.0", .capabilities = {.dimming = false, .pressure = false}};
    } else {
        systemInfo = SystemInfo{.hardware = doc["hw"].as<String>(),
                                .version = doc["v"].as<String>(),
                                .capabilities = SystemCapabilities{
                                    .dimming = doc["cp"]["dm"].as<bool>(),
                                    .pressure = doc["cp"]["ps"].as<bool>(),
                                }};
    }
}

void Controller::setupWifi() {
    if (settings.getWifiSsid() != "" && settings.getWifiPassword() != "") {
        WiFi.mode(WIFI_STA);
        WiFi.begin(settings.getWifiSsid(), settings.getWifiPassword());
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
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

            configTzTime(resolve_timezone(settings.getTimezone()), NTP_SERVER);
        } else {
            WiFi.disconnect(true, true);
            Serial.println("Timed out while connecting to WiFi");
        }
    }
    if (WiFi.status() != WL_CONNECTED) {
        isApConnection = true;
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_IP, WIFI_SUBNET_MASK);
        WiFi.softAP(WIFI_AP_SSID);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
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
    pluginManager->loop();

    if (screenReady) {
        connect();
    }

    if (clientController.isReadyForConnection()) {
        clientController.connectToServer();
        setupInfos();
        pluginManager->trigger("controller:bluetooth:connect");
        if (!loaded) {
            loaded = true;
            if (settings.getStartupMode() == MODE_STANDBY)
                activateStandby();
            pluginManager->trigger("controller:ready");
        }
    }

    unsigned long now = millis();
    if (now - lastPing > PING_INTERVAL) {
        lastPing = now;
        clientController.sendPing();
    }

    if (isErrorState()) {
        return;
    }

    if (now - lastProgress > PROGRESS_INTERVAL) {
        if (currentProcess != nullptr) {
            currentProcess->progress();
            if (!isActive()) {
                deactivate();
            }
        }
        if (lastProcess != nullptr && !lastProcess->isComplete()) {
            lastProcess->progress();
        }
        if (lastProcess != nullptr && lastProcess->isComplete() && !processCompleted && settings.isDelayAdjust()) {
            processCompleted = true;
            if (lastProcess->getType() == MODE_BREW) {
                if (auto const *brewProcess = static_cast<BrewProcess *>(lastProcess);
                    brewProcess->target == ProcessTarget::VOLUMETRIC) {
                    settings.setBrewDelay(brewProcess->getNewDelayTime());
                }
            } else if (lastProcess->getType() == MODE_GRIND) {
                if (auto const *grindProcess = static_cast<GrindProcess *>(lastProcess);
                    grindProcess->target == ProcessTarget::VOLUMETRIC) {
                    settings.setGrindDelay(grindProcess->getNewDelayTime());
                }
            }
        }
        int targetTemp = getTargetTemp();
        if (targetTemp > 0) {
            targetTemp = targetTemp + settings.getTemperatureOffset();
        }
        clientController.sendTemperatureControl(targetTemp);
        clientController.sendPidSettings(settings.getPid());
        updateRelay();
        lastProgress = now;
    }

    if (grindActiveUntil != 0 && now > grindActiveUntil)
        deactivateGrind();
    if (mode != MODE_STANDBY && now > lastAction + settings.getStandbyTimeout())
        activateStandby();
}

bool Controller::isUpdating() const { return updating; }

bool Controller::isAutotuning() const { return autotuning; }

bool Controller::isReady() const { return !isUpdating() && !isErrorState() && !isAutotuning(); }

void Controller::autotune(int testTime, int samples) {
    if (isActive() || !isReady()) {
        return;
    }
    if (mode != MODE_STANDBY) {
        activateStandby();
    }
    autotuning = true;
    clientController.sendAutotune(testTime, samples);
    pluginManager->trigger("controller:autotune:start");
}

void Controller::startProcess(Process *process) {
    if (isActive() || !isReady())
        return;
    processCompleted = false;
    this->currentProcess = process;
    updateRelay();
    updateLastAction();
}

int Controller::getTargetTemp() {
    if (isAutotuning()) {
        return settings.getTargetBrewTemp();
    }

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
    int targetTemp = getTargetTemp();
    if (targetTemp > 0) {
        targetTemp = targetTemp + settings.getTemperatureOffset();
    }
    clientController.sendTemperatureControl(targetTemp);
    pluginManager->trigger("boiler:targetTemperature:change", "value", getTargetTemp());
}

int Controller::getTargetDuration() const { return settings.getTargetDuration(); }

void Controller::setTargetDuration(int duration) {
    Event event = pluginManager->trigger("controller:targetDuration:change", "value", duration);
    settings.setTargetDuration(event.getInt("value"));
    updateLastAction();
}

void Controller::setTargetVolume(int volume) {
    Event event = pluginManager->trigger("controller:targetVolume:change", "value", volume);
    settings.setTargetVolume(event.getInt("value"));
    updateLastAction();
}

int Controller::getTargetGrindDuration() const { return settings.getTargetGrindDuration(); }

void Controller::setTargetGrindDuration(int duration) {
    Event event = pluginManager->trigger("controller:grindDuration:change", "value", duration);
    settings.setTargetGrindDuration(event.getInt("value"));
    updateLastAction();
}

void Controller::setTargetGrindVolume(int volume) {
    Event event = pluginManager->trigger("controller:grindVolume:change", "value", volume);
    settings.setTargetGrindVolume(event.getInt("value"));
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
    if (settings.isVolumetricTarget() && isVolumetricAvailable()) {
        int newTarget = settings.getTargetVolume() + 1;
        if (newTarget > BREW_MAX_VOLUMETRIC) {
            newTarget = BREW_MAX_VOLUMETRIC;
        }
        setTargetVolume(newTarget);
    } else {
        int newDuration = getTargetDuration() + 1000;
        if (newDuration > BREW_MAX_DURATION_MS) {
            newDuration = BREW_MIN_DURATION_MS;
        }
        setTargetDuration(newDuration);
    }
}

void Controller::lowerBrewTarget() {
    if (settings.isVolumetricTarget() && isVolumetricAvailable()) {
        int newTarget = settings.getTargetVolume() - 1;
        if (newTarget < BREW_MIN_VOLUMETRIC) {
            newTarget = BREW_MIN_VOLUMETRIC;
        }
        setTargetVolume(newTarget);
    } else {
        int newDuration = getTargetDuration() - 1000;
        if (newDuration < BREW_MIN_DURATION_MS) {
            newDuration = BREW_MIN_DURATION_MS;
        }
        setTargetDuration(newDuration);
    }
}

void Controller::raiseGrindTarget() {
    if (settings.isVolumetricTarget() && isVolumetricAvailable()) {
        int newTarget = settings.getTargetGrindVolume() + 1;
        if (newTarget > BREW_MAX_VOLUMETRIC) {
            newTarget = BREW_MAX_VOLUMETRIC;
        }
        setTargetGrindVolume(newTarget);
    } else {
        int newDuration = getTargetGrindDuration() + 1000;
        if (newDuration > BREW_MAX_DURATION_MS) {
            newDuration = BREW_MAX_DURATION_MS;
        }
        setTargetGrindDuration(newDuration);
    }
}

void Controller::lowerGrindTarget() {
    if (settings.isVolumetricTarget() && isVolumetricAvailable()) {
        int newTarget = settings.getTargetGrindVolume() - 1;
        if (newTarget < BREW_MIN_VOLUMETRIC) {
            newTarget = BREW_MIN_VOLUMETRIC;
        }
        setTargetGrindVolume(newTarget);
    } else {
        int newDuration = getTargetGrindDuration() - 1000;
        if (newDuration < BREW_MIN_DURATION_MS) {
            newDuration = BREW_MIN_DURATION_MS;
        }
        setTargetGrindDuration(newDuration);
    }
}

void Controller::updateRelay() {
    clientController.sendPumpControl(isActive() ? currentProcess->getPumpValue() : 0);
    clientController.sendValveControl(isActive() && currentProcess->isRelayActive());
    clientController.sendAltControl(isActive() && currentProcess->isAltRelayActive());
}

void Controller::activate() {
    if (isActive())
        return;
    clear();
    switch (mode) {
    case MODE_BREW:
        if (settings.isVolumetricTarget() && volumetricAvailable) {
            startProcess(new BrewProcess(ProcessTarget::VOLUMETRIC, settings.getPressurizeTime(), settings.getInfusePumpTime(),
                                         settings.getInfuseBloomTime(), 0, settings.getTargetVolume(), settings.getBrewDelay()));
        } else {
            startProcess(new BrewProcess(ProcessTarget::TIME, settings.getPressurizeTime(), settings.getInfusePumpTime(),
                                         settings.getInfuseBloomTime(), settings.getTargetDuration(), 0, 0.0));
        }
        break;
    case MODE_STEAM:
        startProcess(new SteamProcess());
        break;
    case MODE_WATER:
        startProcess(new PumpProcess());
        break;
    default:;
    }
    if (currentProcess->getType() == MODE_BREW) {
        pluginManager->trigger("controller:brew:start");
    }
}

void Controller::deactivate() {
    if (currentProcess == nullptr) {
        return;
    }
    delete lastProcess;
    lastProcess = currentProcess;
    currentProcess = nullptr;
    if (lastProcess->getType() == MODE_BREW) {
        pluginManager->trigger("controller:brew:end");
    }
    if (lastProcess->getType() == MODE_GRIND) {
        pluginManager->trigger("controller:grind:end");
    }
    updateRelay();
    updateLastAction();
}

void Controller::clear() {
    processCompleted = true;
    if (lastProcess != nullptr && lastProcess->getType() == MODE_BREW) {
        pluginManager->trigger("controller:brew:clear");
    }
    delete lastProcess;
    lastProcess = nullptr;
}

void Controller::activateGrind() {
    pluginManager->trigger("controller:grind:start");
    if (isGrindActive())
        return;
    clear();
    if (settings.isVolumetricTarget() && volumetricAvailable) {
        startProcess(new GrindProcess(ProcessTarget::VOLUMETRIC, 0, settings.getTargetGrindVolume(), settings.getGrindDelay()));
    } else {
        startProcess(
            new GrindProcess(ProcessTarget::TIME, settings.getTargetGrindDuration(), settings.getTargetGrindVolume(), 0.0));
    }
}

void Controller::deactivateGrind() {
    deactivate();
    clear();
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

bool Controller::isGrindActive() const { return isActive() && currentProcess->getType() == MODE_GRIND; }

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

void Controller::onVolumetricMeasurement(double measurement) const {
    if (currentProcess != nullptr) {
        currentProcess->updateVolume(measurement);
    }
    if (lastProcess != nullptr) {
        lastProcess->updateVolume(measurement);
    }
}

void Controller::handleBrewButton(int brewButtonStatus) {
    printf("current screen %d, brew button %d\n", getMode(), brewButtonStatus);
    if (brewButtonStatus) {
        switch (getMode()) {
        case MODE_STANDBY:
            deactivateStandby();
            break;
        case MODE_BREW:
            if (!isActive()) {
                deactivateStandby();
                clear();
                activate();
            }
            break;
        case MODE_WATER:
            activate();
            break;
        default:
            break;
        }
    } else if (!settings.isMomentaryButtons()) {
        if (getMode() == MODE_BREW) {
            if (isActive()) {
                deactivate();
                clear();
            } else {
                clear();
            }
        } else if (getMode() == MODE_WATER) {
            deactivate();
        }
    }
}

void Controller::handleSteamButton(int steamButtonStatus) {
    printf("current screen %d, steam button %d\n", getMode(), steamButtonStatus);
    if (steamButtonStatus) {
        switch (getMode()) {
        case MODE_STANDBY:
            setMode(MODE_STEAM);
            break;
        case MODE_BREW:
            setMode(MODE_STEAM);
            activate();
            break;
        case MODE_STEAM:
            activate();
            break;
        default:
            break;
        }
    } else if (!settings.isMomentaryButtons() && getMode() == MODE_STEAM) {
        deactivate();
    }
}
