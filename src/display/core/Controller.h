#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "NimBLEClientController.h"
#include "PluginManager.h"
#include "Settings.h"
#include <WiFi.h>
#include <display/ui/default/DefaultUI.h>

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
    int getTargetDuration() const;
    void setTargetDuration(int duration);
    int getTargetGrindDuration() const;
    void setTargetGrindDuration(int duration);
    virtual int getCurrentTemp() const { return currentTemp; }
    bool isActive() const;
    bool isGrindActive() const;
    bool isUpdating() const;
    unsigned long getActiveUntil() const { return activeUntil; }
    Settings &getSettings() { return settings; }
    DefaultUI* getUI() const { return ui; }

    // Event callback methods
    void updateLastAction();
    void raiseTemp();
    void lowerTemp();
    void activate();
    void deactivate();
    void activateGrind();
    void deactivateGrind();
    void activateStandby();
    void deactivateStandby();
    void onOTAUpdate();
    void onScreenReady();

  private:
    // Initialization methods
    void setupPanel();
    void setupWifi();
    void setupBluetooth();

    // Functional methods
    void updateRelay();

    // Event handlers
    void onTempRead(float temperature);

    // Private Attributes
    DefaultUI *ui = nullptr;
    NimBLEClientController clientController;
    hw_timer_t *timer;
    Settings settings;
    PluginManager *pluginManager{};

    int mode;
    int currentTemp;

    unsigned long activeUntil;
    unsigned long grindActiveUntil;
    unsigned long lastPing;
    unsigned long lastProgress;
    unsigned long lastAction;
    bool loaded;
    bool updating;
    bool isApConnection = false;
    bool initialized = false;
    bool screenReady = false;
};

#endif // CONTROLLER_H
