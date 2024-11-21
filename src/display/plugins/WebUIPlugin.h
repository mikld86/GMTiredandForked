#ifndef WEBUIPLUGIN_H
#define WEBUIPLUGIN_H

#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include "../core/Plugin.h"
#include <ESPAsyncWebServer.h>

class WebUIPlugin : public Plugin {
  public:
    WebUIPlugin();
    void setup(Controller *controller, PluginManager *pluginManager) override;
    void loop() override {};
  private:
    void start();
    String processTemplate(const String& var);

    AsyncWebServer server;
    Controller *controller;
};

#endif // WEBUIPLUGIN_H
