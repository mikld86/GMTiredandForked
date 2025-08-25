#include "ShotHistoryPlugin.h"

#include <SPIFFS.h>
#include <display/core/Controller.h>
#include <display/core/ProfileManager.h>
#include <display/core/utils.h>

ShotHistoryPlugin ShotHistory;

void ShotHistoryPlugin::setup(Controller *c, PluginManager *pm) {
    controller = c;
    pluginManager = pm;
    pm->on("controller:brew:start", [this](Event const &) { startRecording(); });
    pm->on("controller:brew:end", [this](Event const &) { endRecording(); });
    pm->on("controller:volumetric-measurement:estimation:change",
           [this](Event const &event) { currentEstimatedWeight = event.getFloat("value"); });
    pm->on("controller:volumetric-measurement:bluetooth:change", [this](Event const &event) {
        const float weight = event.getFloat("value");
        const unsigned long now = millis();
        if (lastVolumeSample != 0) {
            const unsigned long timeDiff = now - lastVolumeSample;
            const float volumeDiff = weight - currentBluetoothWeight;
            const float volumeFlow = volumeDiff / static_cast<float>(timeDiff) * 1000.0f;
            currentBluetoothFlow = currentBluetoothFlow * 0.9f + volumeFlow * 0.1f;
        }
        lastVolumeSample = now;
        currentBluetoothWeight = weight;
    });
    pm->on("boiler:currentTemperature:change", [this](Event const &event) { currentTemperature = event.getFloat("value"); });
    xTaskCreatePinnedToCore(loopTask, "ShotHistoryPlugin::loop", configMINIMAL_STACK_SIZE * 3, this, 1, &taskHandle, 0);
}

void ShotHistoryPlugin::record() {
    static File file;
    if (recording && controller->getMode() == MODE_BREW) {
        if (!isFileOpen) {
            if (!SPIFFS.exists("/h")) {
                SPIFFS.mkdir("/h");
            }
            file = SPIFFS.open("/h/" + currentId + ".dat", FILE_APPEND);
            if (file) {
                isFileOpen = true;
            }
        }
        if (!headerWritten) {
            file.printf("1,%s,%ld\n", currentProfileName.c_str(), getTime());
            headerWritten = true;
        }
        ShotSample s{millis() - shotStart,
                     controller->getTargetTemp(),
                     currentTemperature,
                     controller->getTargetPressure(),
                     controller->getCurrentPressure(),
                     controller->getCurrentPumpFlow(),
                     controller->getTargetFlow(),
                     controller->getCurrentPuckFlow(),
                     currentBluetoothFlow,
                     currentBluetoothWeight,
                     currentEstimatedWeight};
        if (isFileOpen) {
            file.println(s.serialize().c_str());
        }
    }
    if (!recording && isFileOpen) {
        file.close();
        isFileOpen = false;
        unsigned long duration = millis() - shotStart;
        if (duration <= 7500) { // Exclude failed shots and flushes
            SPIFFS.remove("/h/" + currentId + ".dat");
            SPIFFS.remove("/h/" + currentId + ".json"); // Also remove notes file if it exists
        } else {
            controller->getSettings().setHistoryIndex(controller->getSettings().getHistoryIndex() + 1);
            cleanupHistory();
        }
    }
}

void ShotHistoryPlugin::startRecording() {
    currentId = controller->getSettings().getHistoryIndex();
    while (currentId.length() < 6) {
        currentId = "0" + currentId;
    }
    shotStart = millis();
    lastVolumeSample = 0;
    currentBluetoothWeight = 0.0f;
    currentEstimatedWeight = 0.0f;
    currentBluetoothFlow = 0.0f;
    currentProfileName = controller->getProfileManager()->getSelectedProfile().label;
    recording = true;
    headerWritten = false;
}

unsigned long ShotHistoryPlugin::getTime() {
    time_t now;
    time(&now);
    return now;
}

void ShotHistoryPlugin::endRecording() { recording = false; }

void ShotHistoryPlugin::cleanupHistory() {
    File directory = SPIFFS.open("/h");
    std::vector<String> entries;
    String filename = directory.getNextFileName();
    while (filename != "") {
        entries.push_back(filename);
        filename = directory.getNextFileName();
    }
    sort(entries.begin(), entries.end(), [](String a, String b) { return a < b; });
    if (entries.size() > MAX_HISTORY_ENTRIES) {
        for (unsigned int i = 0; i < entries.size() - MAX_HISTORY_ENTRIES; i++) {
            String name = entries[i];
            SPIFFS.remove(name);
        }
    }
}

void ShotHistoryPlugin::handleRequest(JsonDocument &request, JsonDocument &response) {
    String type = request["tp"].as<String>();
    response["tp"] = String("res:") + type.substring(4);
    response["rid"] = request["rid"].as<String>();

    if (type == "req:history:list") {
        JsonArray arr = response["history"].to<JsonArray>();
        File root = SPIFFS.open("/h");
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                if (String(file.name()).endsWith(".dat")) {
                    auto o = arr.add<JsonObject>();
                    auto name = String(file.name());
                    int start = name.lastIndexOf('/') + 1;
                    int end = name.lastIndexOf('.');
                    String id = name.substring(start, end);
                    o["id"] = id;
                    o["history"] = file.readString();
                    
                    // Also include notes if they exist
                    JsonDocument notes;
                    loadNotes(id, notes);
                    if (!notes.isNull() && notes.size() > 0) {
                        o["notes"] = notes;
                    }
                }
                file = root.openNextFile();
            }
        }
    } else if (type == "req:history:get") {
        auto id = request["id"].as<String>();
        File file = SPIFFS.open("/h/" + id + ".dat", "r");
        if (file) {
            String data = file.readString();
            response["history"] = data;
            file.close();
            
            // Also include notes if they exist
            JsonDocument notes;
            loadNotes(id, notes);
            if (!notes.isNull() && notes.size() > 0) {
                response["notes"] = notes;
            }
        } else {
            response["error"] = "not found";
        }
    } else if (type == "req:history:delete") {
        auto id = request["id"].as<String>();
        SPIFFS.remove("/h/" + id + ".dat");
        SPIFFS.remove("/h/" + id + ".json"); // Also remove notes file if it exists
        response["msg"] = "Ok";
    } else if (type == "req:history:notes:get") {
        auto id = request["id"].as<String>();
        JsonDocument notes;
        loadNotes(id, notes);
        response["notes"] = notes;
    } else if (type == "req:history:notes:save") {
        const String id = request["id"].as<String>();
        const JsonDocument& notesDoc = request["notes"];
        
        saveNotes(id, notesDoc);

    } 
}

void ShotHistoryPlugin::saveNotes(const String &id, const JsonDocument &notes) {
    File file = SPIFFS.open("/h/" + id + ".json", FILE_WRITE);
    if (file) {
        String notesStr;
        serializeJson(notes, notesStr);
        file.print(notesStr);
        file.close();
    }
}

void ShotHistoryPlugin::loadNotes(const String &id, JsonDocument &notes) {
    File file = SPIFFS.open("/h/" + id + ".json", "r");
    if (file) {
        String notesStr = file.readString();
        file.close();
        deserializeJson(notes, notesStr);
    }
}

void ShotHistoryPlugin::loopTask(void *arg) {
    auto *plugin = static_cast<ShotHistoryPlugin *>(arg);
    while (true) {
        plugin->record();
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}
