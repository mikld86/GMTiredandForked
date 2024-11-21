#include "mDNSPlugin.h"
#include "../core/Controller.h"
#include <ESPmDNS.h>

void mDNSPlugin::setup(Controller *controller, PluginManager *pluginManager) {
    if (!MDNS.begin(controller->getSettings().getMdnsName().c_str())) {
        Serial.println("Error setting up MDNS responder!");
    }
}
