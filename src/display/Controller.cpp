#include "Controller.h"

Controller::Controller()
    : server(80), timer(nullptr), mode(MODE_BREW), targetBrewTemp(90), targetSteamTemp(145), targetWaterTemp(80),
      targetDuration(25000), currentTemp(0), activeUntil(0), lastPing(0), lastProgress(0), lastAction(0), loaded(false),
      updating(false), temperatureOffset(0), startupMode(MODE_STANDBY), pid(DEFAULT_PID), wifiSsid(""), wifiPassword("") {}

void Controller::setup() {
    preferences.begin("controller", true);
    startupMode = preferences.getInt("sm", MODE_STANDBY);
    targetBrewTemp = preferences.getInt("tb", 90);
    targetSteamTemp = preferences.getInt("ts", 145);
    targetWaterTemp = preferences.getInt("tw", 80);
    targetDuration = preferences.getInt("td", 25000);
    temperatureOffset = preferences.getInt("to", DEFAULT_TEMPERATURE_OFFSET);
    pid = preferences.getString("pid", DEFAULT_PID);
    wifiSsid = preferences.getString("ws", "");
    wifiPassword = preferences.getString("wp", "");
    preferences.end();
    mode = startupMode;
    setupPanel();
}

void Controller::connect() {
    lastPing = millis();

    setupBluetooth();
    setupWifi();

    updateUiSettings();
    updateUiCurrentTemp();
}

void Controller::setupBluetooth() {
    clientController.initClient();
    clientController.registerTempReadCallback([this](float temp) { onTempRead(temp); });
}

void Controller::setupPanel() {
    // Initialize T-RGB, if the initialization fails, false will be returned.
    if (!panel.begin()) {
        while (1) {
            Serial.println("Error, failed to initialize T-RGB");
            delay(1000);
        }
    }
    beginLvglHelper(panel);
    panel.setBrightness(16);
    ui_init();
}

void Controller::setupWifi() {
    if (wifiSsid != "" && wifiPassword != "") {
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiSsid, wifiPassword);
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
            Serial.println(wifiSsid);
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            configTzTime(TIMEZONE, NTP_SERVER);

#ifdef HOMEKIT_ENABLED
            setupHomekit();
#else
            if (!MDNS.begin(MDNS_NAME)) {
                Serial.println("Error setting up MDNS responder!");
            }
#endif
        } else {
            Serial.println("Timed out while connecting to WiFi");
        }
    }
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_SSID);
        Serial.println("Started in AP mode");
        Serial.print("Connect to:");
        Serial.println(WIFI_SSID);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }

    server.on("/", [this]() {
        std::map<String, String> variables = {{"build_version", BUILD_GIT_VERSION}, {"build_timestamp", BUILD_TIMESTAMP}};
        server.send(200, "text/html", TemplateTango::render(index_html, variables));
    });
    server.on("/settings", [this]() {
        if (server.method() == HTTP_POST) {
            if (server.hasArg("startupMode"))
                startupMode = server.arg("startupMode") == "brew" ? MODE_BREW : MODE_STANDBY;
            if (server.hasArg("targetBrewTemp"))
                targetBrewTemp = server.arg("targetBrewTemp").toInt();
            if (server.hasArg("targetSteamTemp"))
                targetSteamTemp = server.arg("targetSteamTemp").toInt();
            if (server.hasArg("targetWaterTemp"))
                targetWaterTemp = server.arg("targetWaterTemp").toInt();
            if (server.hasArg("targetDuration"))
                targetDuration = server.arg("targetDuration").toInt() * 1000;
            if (server.hasArg("temperatureOffset"))
                temperatureOffset = server.arg("temperatureOffset").toInt();
            if (server.hasArg("pid"))
                pid = server.arg("pid");
            if (server.hasArg("wifiSsid"))
                wifiSsid = server.arg("wifiSsid");
            if (server.hasArg("wifiPassword"))
                wifiPassword = server.arg("wifiPassword");
            setTargetTemp(getTargetTemp());
        }

        std::map<String, String> variables = {{"standbySelected", startupMode == MODE_STANDBY ? "selected" : ""},
                                              {"brewSelected", startupMode == MODE_BREW ? "selected" : ""},
                                              {"targetBrewTemp", String(targetBrewTemp)},
                                              {"targetSteamTemp", String(targetSteamTemp)},
                                              {"targetWaterTemp", String(targetWaterTemp)},
                                              {"targetDuration", String(targetDuration / 1000)},
                                              {"pid", pid},
                                              {"wifiSsid", wifiSsid},
                                              {"wifiPassword", wifiPassword},
                                              {"build_version", BUILD_GIT_VERSION},
                                              {"build_timestamp", BUILD_TIMESTAMP},
                                              {"temperatureOffset", String(temperatureOffset)}};
        server.send(200, "text/html", TemplateTango::render(settings_html, variables));
        if (server.method() == HTTP_POST && server.hasArg("restart"))
            ESP.restart();
    });
    ElegantOTA.begin(&server);
    ElegantOTA.onStart([this]() { onOTAUpdate(); });
    server.begin();
    Serial.print("OTA server started");
}

#ifdef HOMEKIT_ENABLED
void Controller::setupHomekit() {
    homekitController.initialize(wifiSsid, wifiPassword);
    homekitController.setTargetTemperature(getTargetTemp());
}
#endif

void Controller::loop() {
    server.handleClient();
    ElegantOTA.loop();

    if (clientController.isReadyForConnection()) {
        clientController.connectToServer();
        if (!loaded) {
            loaded = true;
            startupMode == MODE_BREW
                ? _ui_screen_change(&ui_BrewScreen, LV_SCR_LOAD_ANIM_NONE, 500, 0, &ui_BrewScreen_screen_init)
                : activateStandby();
        }
    }

    unsigned long now = millis();
    if (now - lastPing > PING_INTERVAL) {
        lastPing = now;
        clientController.sendPing();
    }

    if (now - lastProgress > PROGRESS_INTERVAL) {
        if (mode == MODE_BREW)
            updateBrewProgress();
        else if (mode == MODE_STANDBY)
            updateStandby();
        clientController.sendTemperatureControl(getTargetTemp() + temperatureOffset);
        clientController.sendPidSettings(pid);
        updateRelay();
        lastProgress = now;
    }

    if (activeUntil != 0 && now > activeUntil)
        deactivate();
    if (mode != MODE_STANDBY && now > lastAction + STANDBY_TIMEOUT_MS)
        activateStandby();

#ifdef HOMEKIT_ENABLED
    if (homekitController.hasAction()) {
        if (homekitController.getState() && getMode() == MODE_STANDBY) {
            deactivate();
            setMode(MODE_BREW);
            _ui_screen_change(&ui_BrewScreen, LV_SCR_LOAD_ANIM_NONE, 500, 0, &ui_BrewScreen_screen_init);
        } else if (!homekitController.getState() && getMode() != MODE_STANDBY) {
            activateStandby();
        }
        setTargetTemp(homekitController.getTargetTemperature());
        homekitController.clearAction();
    }
#endif
}

bool Controller::isUpdating() const { return updating; }

int Controller::getTargetTemp() {
    switch (mode) {
    case MODE_BREW:
        return targetBrewTemp;
    case MODE_STEAM:
        return targetSteamTemp;
    case MODE_WATER:
        return targetWaterTemp;
    default:
        return 0;
    }
}

void Controller::setTargetTemp(int temperature) {
    switch (mode) {
    case MODE_BREW:
        targetBrewTemp = temperature;
        break;
    case MODE_STEAM:
        targetSteamTemp = temperature;
        break;
    case MODE_WATER:
        targetWaterTemp = temperature;
        break;
    }
    updateUiSettings();
    clientController.sendPidSettings(pid);
    clientController.sendTemperatureControl(getTargetTemp() + temperatureOffset);
    savePreferences();
#ifdef HOMEKIT_ENABLED
    homekitController.setTargetTemperature(getTargetTemp());
#endif
}

int Controller::getTargetDuration() { return targetDuration; }

void Controller::setTargetDuration(int duration) {
    targetDuration = duration;
    updateUiSettings();
    savePreferences();
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

void Controller::updateRelay() {
    bool active = isActive();
    float pumpValue = active ? mode == MODE_STEAM ? 4 : 100 : 0;
    bool valve = (active && mode == MODE_BREW);

    clientController.sendPumpControl(pumpValue);
    clientController.sendValveControl(valve);
}

void Controller::updateUiActive() {
    bool active = isActive();
    if (mode == MODE_BREW) {
        if (!active) {
            _ui_screen_change(&ui_BrewScreen, LV_SCR_LOAD_ANIM_NONE, 500, 0, &ui_BrewScreen_screen_init);
        }
    }
    lv_imgbtn_set_src(ui_SteamScreen_goButton, LV_IMGBTN_STATE_RELEASED, NULL, active ? &ui_img_646127855 : &ui_img_2106667244,
                      NULL);
    lv_imgbtn_set_src(ui_WaterScreen_goButton, LV_IMGBTN_STATE_RELEASED, NULL, active ? &ui_img_646127855 : &ui_img_2106667244,
                      NULL);
}

void Controller::updateUiSettings() {
    int setTemp = getTargetTemp();
    lv_arc_set_value(ui_BrewScreen_tempTarget, setTemp);
    lv_arc_set_value(ui_StatusScreen_tempTarget, setTemp);
    lv_arc_set_value(ui_MenuScreen_tempTarget, setTemp);
    lv_arc_set_value(ui_SteamScreen_tempTarget, setTemp);
    lv_arc_set_value(ui_WaterScreen_tempTarget, setTemp);

    lv_label_set_text_fmt(ui_StatusScreen_targetTemp, "%d°C", targetBrewTemp);
    lv_label_set_text_fmt(ui_BrewScreen_targetTemp, "%d°C", targetBrewTemp);
    lv_label_set_text_fmt(ui_SteamScreen_targetTemp, "%d°C", targetSteamTemp);
    lv_label_set_text_fmt(ui_WaterScreen_targetTemp, "%d°C", targetWaterTemp);

    double secondsDouble = targetDuration / 1000.0;
    int minutes = (int)(secondsDouble / 60.0 - 0.5);
    int seconds = (int)secondsDouble % 60;
    lv_label_set_text_fmt(ui_BrewScreen_targetDuration, "%2d:%02d", minutes, seconds);
    lv_label_set_text_fmt(ui_StatusScreen_targetDuration, "%2d:%02d", minutes, seconds);

    updateLastAction();
}

void Controller::updateUiCurrentTemp() {
    int temp = currentTemp - temperatureOffset;
    lv_arc_set_value(ui_BrewScreen_tempGauge, temp);
    lv_arc_set_value(ui_StatusScreen_tempGauge, temp);
    lv_arc_set_value(ui_MenuScreen_tempGauge, temp);
    lv_arc_set_value(ui_SteamScreen_tempGauge, temp);
    lv_arc_set_value(ui_WaterScreen_tempGauge, temp);

    lv_label_set_text_fmt(ui_BrewScreen_tempText, "%d°C", temp);
    lv_label_set_text_fmt(ui_StatusScreen_tempText, "%d°C", temp);
    lv_label_set_text_fmt(ui_MenuScreen_tempText, "%d°C", temp);
    lv_label_set_text_fmt(ui_SteamScreen_tempText, "%d°C", temp);
    lv_label_set_text_fmt(ui_WaterScreen_tempText, "%d°C", temp);
}

void Controller::updateBrewProgress() {
    unsigned long now = millis();
    unsigned long progress = now - (activeUntil - targetDuration);
    double secondsDouble = targetDuration / 1000.0;
    int minutes = (int)(secondsDouble / 60.0 - 0.5);
    int seconds = (int)secondsDouble % 60;
    double progressSecondsDouble = progress / 1000.0;
    int progressMinutes = (int)(progressSecondsDouble / 60.0 - 0.5);
    int progressSeconds = (int)progressSecondsDouble % 60;
    lv_bar_set_range(ui_StatusScreen_progressBar, 0, (int)secondsDouble);
    lv_bar_set_value(ui_StatusScreen_progressBar, progress / 1000, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_StatusScreen_progressLabel, "%2d:%02d / %2d:%02d", progressMinutes, progressSeconds, minutes,
                          seconds);
}

void Controller::updateStandby() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char time[6];
        strftime(time, 6, "%H:%M", &timeinfo);
        lv_label_set_text(ui_StandbyScreen_time, time);
    }
    ui_object_set_themeable_style_property(ui_StandbyScreen_bluetoothIcon, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_IMG_RECOLOR,
                                           clientController.isConnected() ? _ui_theme_color_NiceWhite : _ui_theme_color_SemiDark);
    ui_object_set_themeable_style_property(ui_StandbyScreen_wifiIcon, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_IMG_RECOLOR,
                                           WiFi.status() == WL_CONNECTED ? _ui_theme_color_NiceWhite : _ui_theme_color_SemiDark);
}

void Controller::activate() {
    if (isActive())
        return;
    unsigned long duration = 0;
    switch (mode) {
    case MODE_BREW:
        duration = targetDuration;
        break;
    case MODE_STEAM:
        duration = STEAM_SAFETY_DURATION_MS;
        break;
    case MODE_WATER:
        duration = HOT_WATER_SAFETY_DURATION_MS;
        break;
    }
    activeUntil = millis() + duration;
    updateUiActive();
    updateRelay();
    updateLastAction();
}

void Controller::deactivate() {
    activeUntil = 0;
    updateUiActive();
    updateRelay();
    updateLastAction();
}

void Controller::activateStandby() {
    setMode(MODE_STANDBY);
    deactivate();
    _ui_screen_change(&ui_StandbyScreen, LV_SCR_LOAD_ANIM_NONE, 500, 0, &ui_StandbyScreen_screen_init);
}

bool Controller::isActive() const { return activeUntil > millis(); }

int Controller::getMode() { return mode; }

void Controller::setMode(int newMode) {
    mode = newMode;
    updateUiSettings();
    clientController.sendTemperatureControl(getTargetTemp() + temperatureOffset);

#ifdef HOMEKIT_ENABLED
    homekitController.setState(mode != MODE_STANDBY);
    homekitController.setTargetTemperature(getTargetTemp());
#endif
}

void Controller::onTempRead(float temperature) {
    currentTemp = temperature;
    updateUiCurrentTemp();

#ifdef HOMEKIT_ENABLED
    homekitController.setCurrentTemperature(temperature - temperatureOffset);
#endif
}

void Controller::updateLastAction() { lastAction = millis(); }

void Controller::onOTAUpdate() {
    activateStandby();
    updating = true;
}

void Controller::savePreferences() {
    preferences.begin("controller", false);
    preferences.putInt("sm", startupMode);
    preferences.putInt("tb", targetBrewTemp);
    preferences.putInt("ts", targetSteamTemp);
    preferences.putInt("tw", targetWaterTemp);
    preferences.putInt("td", targetDuration);
    preferences.putInt("to", temperatureOffset);
    preferences.putString("pid", pid);
    preferences.putString("ws", wifiSsid);
    preferences.putString("wp", wifiPassword);
    preferences.end();
}
