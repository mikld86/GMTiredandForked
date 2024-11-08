#include "mDNSPlugin.h"

void mDNSPlugin::setup(Controller *controller, PluginManager *pluginManager) {
    if (!MDNS.begin(MDNS_NAME)) {
        Serial.println("Error setting up MDNS responder!");
    }
}
