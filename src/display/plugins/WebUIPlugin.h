#ifndef WEBUIPLUGIN_H
#define WEBUIPLUGIN_H

#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include "../core/Plugin.h"
#include "GitHubOTA.h"
#include <ESPAsyncWebServer.h>

constexpr size_t UPDATE_CHECK_INTERVAL = 5 * 60 * 1000;

const String RELEASE_URL = "https://github.com/jniebuhr/gaggimate/releases/";

class WebUIPlugin : public Plugin {
  public:
    WebUIPlugin();
    void setup(Controller *controller, PluginManager *pluginManager) override;
    void loop() override;

  private:
    void start();

    void handleOTA(AsyncWebServerRequest *request);
    void handleSettings(AsyncWebServerRequest *request);
    void handleBLEScaleList(AsyncWebServerRequest *request);
    void handleBLEScaleScan(AsyncWebServerRequest *request);
    void handleBLEScaleConnect(AsyncWebServerRequest *request);
    void handleBLEScaleInfo(AsyncWebServerRequest *request);

    GitHubOTA *ota = nullptr;
    AsyncWebServer server;
    Controller *controller = nullptr;
    PluginManager *pluginManager = nullptr;

    long lastUpdateCheck = 0;
    bool updating = false;
};

#endif // WEBUIPLUGIN_H
