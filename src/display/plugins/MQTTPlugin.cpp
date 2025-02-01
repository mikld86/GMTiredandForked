#include "MQTTPlugin.h"
#include "../core/Controller.h"

bool MQTTPlugin::connect(Controller *controller) {
    const Settings settings = controller->getSettings();
    const String ip = settings.getHomeAssistantIP();
    const int haPort = settings.getHomeAssistantPort();
    const String clientId = "GaggiMate";
    const String haUser = settings.getHomeAssistantUser();
    const String haPassword = settings.getHomeAssistantPassword();

    client.begin(ip.c_str(), haPort, net);
    printf("Connecting to MQTT");
    for (int i = 0; i < MQTT_CONNECTION_RETRIES; i++) {
        if (client.connect(clientId.c_str(), haUser.c_str(), haPassword.c_str())) {
            printf("\n");
            return true;
        }
        printf(".");
        delay(MQTT_CONNECTION_DELAY);
    }
    printf("\nConnection to MQTT failed.\n");
    return false;
}

void MQTTPlugin::setup(Controller *controller, PluginManager *pluginManager) {
    pluginManager->on("controller:wifi:connect", [this, controller](const Event&) {
        if (!connect(controller))
            return;
        String mac = WiFi.macAddress();
        mac.replace(":", "_");
        const char* cmac = mac.c_str();
        char topic[50];
        char json[500];
        snprintf(topic, sizeof(topic), "gaggimate/%s/config", cmac);
        snprintf(json, sizeof(json), R"***({"dev":{"ids":"%s","name":"GaggiMate","mf":"GaggiMate","mdl":"GaggiMate","sw":"1.0","sn":"%s","hw":"1.0"},"o":{"name":"GaggiMate","sw":"v0.3.0","url":"https://gaggimate.eu/"},"cmps":{"boiler":{"p":"sensor","device_class":"temperature","unit_of_measurement":"°C","value_template":"{{ value_json.temperature }}","unique_id":"boiler0Tmp","state_topic":"gaggimate/%s/boilers/0/temperature"}},"state_topic":"gaggimate/%s/state","qos":2})***", cmac, cmac, cmac, cmac);
        client.publish(topic, json);
    });

    pluginManager->on("boiler:currentTemperature:change", [this](Event const &event) {
        if (!client.connected())
            return;
        String mac = WiFi.macAddress();
        mac.replace(":", "_");
        const char* cmac = mac.c_str();
        char topic[50];
        char json[50];
        snprintf(topic, sizeof(topic), "gaggimate/%s/boilers/0/temperature", cmac);
        float temp = event.getFloat("value");
        snprintf(json, sizeof(json), R"***({"temperature":%02f})***", temp);
        client.publish(topic, json);
    });

    pluginManager->on("boiler:targetTemperature:change", [this](const Event &event) {
        if (!client.connected())
            return;
        String mac = WiFi.macAddress();
        mac.replace(":", "_");
        const char* cmac = mac.c_str();
        char topic[50];
        char json[50];
        snprintf(topic, sizeof(topic), "gaggimate/%s/boilers/0/targetTemperature", cmac);
        float temp = event.getFloat("value");
        snprintf(json, sizeof(json), R"***({"temperature":%02f})***", temp);
        client.publish(topic, json);
    });
}
