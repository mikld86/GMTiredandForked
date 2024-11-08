#ifndef MDNSPLUGIN_H
#define MDNSPLUGIN_H
#include "../config.h"
#include "../core/Controller.h"
#include "../core/Plugin.h"
#include <ESPmDNS.h>

class mDNSPlugin : public Plugin {
  public:
    void setup(Controller *controller, PluginManager *pluginManager) override;
    void loop() override {};
};

#endif // MDNSPLUGIN_H
