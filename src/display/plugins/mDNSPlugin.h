#ifndef MDNSPLUGIN_H
#define MDNSPLUGIN_H
#include "../core/Plugin.h"

class mDNSPlugin : public Plugin {
  public:
    void setup(Controller *controller, PluginManager *pluginManager) override;
    void loop() override {};
};

#endif // MDNSPLUGIN_H
