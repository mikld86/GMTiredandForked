#include "WebUIPlugin.h"
#include "../core/Controller.h"
#include <ElegantOTA.h>
#include <SPIFFS.h>

WebUIPlugin::WebUIPlugin() : server(80) {}

void WebUIPlugin::setup(Controller *_controller, PluginManager *pluginManager) {
    this->controller = _controller;
    pluginManager->on("controller:wifi:connect", [this](Event const &) { start(); });
}

void WebUIPlugin::start() {
    ota.setReleaseUrl(RELEASE_URL + controller->getSettings().getOTAChannel());
    ota.checkForUpdates();
    server.on("/", [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false, [this](const String &var) { return processTemplate(var); });
    });
    server.on("/ota", [this](AsyncWebServerRequest *request) { handleOTA(request); });
    server.on("/settings", [this](AsyncWebServerRequest *request) { handleSettings(request); });
    server.serveStatic("/assets", SPIFFS, "/assets/");
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.onStart([this]() { controller->onOTAUpdate(); });
    server.begin();
    Serial.print("OTA server started");
}

String WebUIPlugin::processTemplate(const String &var) {
    Settings const &settings = controller->getSettings();
    std::map<String, String> variables = {{"STANDBY_SELECTED", settings.getStartupMode() == MODE_STANDBY ? "selected" : ""},
                                          {"BREW_SELECTED", settings.getStartupMode() == MODE_BREW ? "selected" : ""},
                                          {"TARGET_BREW_TEMP", String(settings.getTargetBrewTemp())},
                                          {"TARGET_STEAM_TEMP", String(settings.getTargetSteamTemp())},
                                          {"TARGET_WATER_TEMP", String(settings.getTargetWaterTemp())},
                                          {"TARGET_DURATION", String(settings.getTargetDuration() / 1000)},
                                          {"HOMEKIT_CHECKED", settings.isHomekit() ? "checked" : ""},
                                          {"PID", settings.getPid()},
                                          {"WIFI_SSID", settings.getWifiSsid()},
                                          {"WIFI_PASSWORD", settings.getWifiPassword()},
                                          {"MDNS_NAME", settings.getMdnsName()},
                                          {"BUILD_VERSION", BUILD_GIT_VERSION},
                                          {"BUILD_TIMESTAMP", BUILD_TIMESTAMP},
                                          {"TEMPERATURE_OFFSET", String(settings.getTemperatureOffset())}};
    if (variables.find(var) != variables.end()) {
        return variables[var];
    }
    return "";
}

String WebUIPlugin::processOTATemplate(const String &var) {
    Settings const &settings = controller->getSettings();
    std::map<String, String> variables = {
        {"UPDATE_AVAILABLE", ota.isUpdateAvailable() ? "Update Available!" : ""},
        {"LATEST_VERSION", ota.getCurrentVersion()},
        {"STABLE_SELECTED", settings.getOTAChannel() == "latest" ? "selected" : ""},
        {"DEV_SELECTED", settings.getOTAChannel() != "latest" ? "selected" : ""},
    };
    if (variables.find(var) != variables.end()) {
        return variables[var];
    }
    return processTemplate(var);
}

void WebUIPlugin::handleOTA(AsyncWebServerRequest *request) {

    if (request->method() == HTTP_POST) {
        if (request->hasArg("channel")) {
            controller->getSettings().setOTAChannel(request->arg("channel") == "stable" ? "latest" : "nightly");
            ota.setReleaseUrl(RELEASE_URL + controller->getSettings().getOTAChannel());
            ota.checkForUpdates();
        }
        if (request->hasArg("update")) {
            ota.update();
        }
    }
    request->send(SPIFFS, "/ota.html", String(), false, [this](const String &var) { return processOTATemplate(var); });
}

void WebUIPlugin::handleSettings(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {
        controller->getSettings().batchUpdate([request](Settings *settings) {
            if (request->hasArg("startupMode"))
                settings->setStartupMode(request->arg("startupMode") == "brew" ? MODE_BREW : MODE_STANDBY);
            if (request->hasArg("targetBrewTemp"))
                settings->setTargetBrewTemp(request->arg("targetBrewTemp").toInt());
            if (request->hasArg("targetSteamTemp"))
                settings->setTargetSteamTemp(request->arg("targetSteamTemp").toInt());
            if (request->hasArg("targetWaterTemp"))
                settings->setTargetWaterTemp(request->arg("targetWaterTemp").toInt());
            if (request->hasArg("targetDuration"))
                settings->setTargetDuration(request->arg("targetDuration").toInt() * 1000);
            if (request->hasArg("temperatureOffset"))
                settings->setTemperatureOffset(request->arg("temperatureOffset").toInt());
            if (request->hasArg("pid"))
                settings->setPid(request->arg("pid"));
            if (request->hasArg("wifiSsid"))
                settings->setWifiSsid(request->arg("wifiSsid"));
            if (request->hasArg("mdnsName"))
                settings->setMdnsName(request->arg("mdnsName"));
            if (request->hasArg("wifiPassword"))
                settings->setWifiPassword(request->arg("wifiPassword"));
            settings->setHomekit(request->hasArg("homekit"));
        });
        controller->setTargetTemp(controller->getTargetTemp());
    }

    request->send(SPIFFS, "/settings.html", String(), false, [this](const String &var) { return processTemplate(var); });
    if (request->method() == HTTP_POST && request->hasArg("restart"))
        ESP.restart();
}
