#ifndef DEFAULTUI_H
#define DEFAULTUI_H

#include <display/core/constants.h>
#include <display/core/PluginManager.h>
#include <display/drivers/LilyGo-T-RGB/LilyGo_RGBPanel.h>

#include "./lvgl/ui.h"

class Controller;

class DefaultUI {
public:
    DefaultUI(Controller *controller, PluginManager *pluginManager);

    // Default work methods
    void init();
    void loop();

    // Interface methods
    void changeScreen(lv_obj_t **screen, void (*target_init)(void));

private:
    void setupPanel();

    void handleScreenChange();

    void updateStandbyScreen() const;
    void updateStatusScreen();
    void updateTemperatures() const;
    void updateDurations();
    void updateActiveStates();

    LilyGo_RGBPanel panel;
    Controller *controller;
    PluginManager *pluginManager;

    // Screen state
    bool updateAvailable = false;
    bool updateActive = false;
    bool bluetoothActive = false;
    bool apActive = false;

    int mode = MODE_STANDBY;

    // Screen change
    lv_obj_t **targetScreen = &ui_InitScreen;
    void (*targetScreenInit)(void) = &ui_InitScreen_screen_init;
};

#endif //DEFAULTUI_H
