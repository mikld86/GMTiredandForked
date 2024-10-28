#include "Controller.h"

Controller::Controller()
    : server(80), timer(nullptr), mode(MODE_BREW), targetBrewTemp(90), targetSteamTemp(145), targetWaterTemp(80),
      targetDuration(25000), currentTemp(0), activeUntil(0), lastPing(0), lastProgress(0), lastAction(0), loaded(false),
      updating(false) {}

void Controller::setup() {
    mode = BREW_ON_START ? MODE_BREW : MODE_STANDBY;
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
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    configTzTime(TIMEZONE, NTP_SERVER);
    server.on("/", [this]() { server.send(200, "text/html", index_html); });
    ElegantOTA.begin(&server);
    ElegantOTA.onStart([this]() { onOTAUpdate(); });
    server.begin();
    Serial.print("OTA server started");

    if (!MDNS.begin(MDNS_NAME)) {
        Serial.println("Error setting up MDNS responder!");
    }
}

void Controller::loop() {
    server.handleClient();
    ElegantOTA.loop();

    if (clientController.isReadyForConnection()) {
        clientController.connectToServer();
        if (!loaded) {
            loaded = true;
            BREW_ON_START ? _ui_screen_change(&ui_BrewScreen, LV_SCR_LOAD_ANIM_NONE, 500, 0, &ui_BrewScreen_screen_init)
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
        clientController.sendTemperatureControl(getTargetTemp());
        updateRelay();
        lastProgress = now;
    }

    if (activeUntil != 0 && now > activeUntil)
        deactivate();
    if (mode != MODE_STANDBY && now > lastAction + STANDBY_TIMEOUT_MS)
        activateStandby();
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
    clientController.sendTemperatureControl(getTargetTemp());
}

int Controller::getTargetDuration() { return targetDuration; }

void Controller::setTargetDuration(int duration) {
    targetDuration = duration;
    updateUiSettings();
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
    lv_arc_set_value(ui_BrewScreen_tempGauge, currentTemp);
    lv_arc_set_value(ui_StatusScreen_tempGauge, currentTemp);
    lv_arc_set_value(ui_MenuScreen_tempGauge, currentTemp);
    lv_arc_set_value(ui_SteamScreen_tempGauge, currentTemp);
    lv_arc_set_value(ui_WaterScreen_tempGauge, currentTemp);

    lv_label_set_text_fmt(ui_BrewScreen_tempText, "%d°C", currentTemp);
    lv_label_set_text_fmt(ui_StatusScreen_tempText, "%d°C", currentTemp);
    lv_label_set_text_fmt(ui_MenuScreen_tempText, "%d°C", currentTemp);
    lv_label_set_text_fmt(ui_SteamScreen_tempText, "%d°C", currentTemp);
    lv_label_set_text_fmt(ui_WaterScreen_tempText, "%d°C", currentTemp);
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
}

void Controller::activate() {
    if (isActive()) return;
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

void Controller::setMode(int newMode) {
    mode = newMode;
    updateUiSettings();
    clientController.sendTemperatureControl(getTargetTemp());
}

void Controller::onTempRead(float temperature) {
    currentTemp = temperature;
    updateUiCurrentTemp();
}

void Controller::updateLastAction() { lastAction = millis(); }

void Controller::onOTAUpdate() {
    activateStandby();
    updating = true;
}
