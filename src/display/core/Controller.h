#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../drivers/LilyGo-T-RGB/LV_Helper.h"
#include "../drivers/LilyGo-T-RGB/LilyGo_RGBPanel.h"
#include "../ui/ui.h"
#include "../web/TemplateTango.h"
#include "../web/html.h"
#include "NimBLEClientController.h"
#include "PluginManager.h"
#include "Settings.h"
#include "config.h"
#include "constants.h"
#include <ElegantOTA.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ctime>

class Controller {
  public:
    Controller();

    // Base methods called from sketch
    void setup();
    void connect();
    void loop(); // Called in loop, encapsulating most of the functionality

    // Getters and setters
    int getMode() const;
    void setMode(int newMode);
    int getTargetTemp();
    void setTargetTemp(int temperature);
    int getTargetDuration();
    void setTargetDuration(int duration);
    bool isActive() const;
    bool isUpdating() const;

    // Event callback methods
    void updateLastAction();
    void raiseTemp();
    void lowerTemp();
    void activate();
    void deactivate();
    void activateStandby();

  private:
    // Initialization methods
    void setupPanel();
    void setupWifi();
    void setupBluetooth();

    // Functional methods
    void updateRelay();
    void updateUiActive() const;
    void updateUiSettings();
    void updateUiCurrentTemp() const;
    void updateBrewProgress() const;
    void updateStandby();

    // Event handlers
    void onOTAUpdate();
    void onTempRead(float temperature);

    // Private Attributes
    WebServer server;
    LilyGo_RGBPanel panel;
    NimBLEClientController clientController;
    hw_timer_t *timer;
    Settings settings;
    PluginManager *pluginManager{};

    int mode;
    int currentTemp;

    unsigned long activeUntil;
    unsigned long lastPing;
    unsigned long lastProgress;
    unsigned long lastAction;
    bool loaded;
    bool updating;
};

#endif // CONTROLLER_H
