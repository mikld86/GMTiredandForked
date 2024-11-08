#include "PluginManager.h"
void PluginManager::registerPlugin(Plugin *plugin) { plugins.push_back(plugin); }

void PluginManager::setup(Controller *controller) {
    for (auto &plugin : plugins) {
        plugin->setup(controller, this);
    }
    initialized = true;
}

void PluginManager::loop() {
    if (!initialized)
        return;
    for (auto &plugin : plugins) {
        plugin->loop();
    }
}

void PluginManager::on(const String &eventId, const EventCallback &callback) { listeners[eventId].push_back(callback); }

Event *PluginManager::trigger(const String &eventId, const EventData &data) {
    Event event;
    event.id = eventId;
    event.data = data;
    trigger(event);
    return &event;
}

Event *PluginManager::trigger(const String &eventId, const String &key, const String &value) {
    EventData data;
    data[key] = EventDataEntry(value);
    return trigger(eventId, data);
}

Event *PluginManager::trigger(const String &eventId, const String &key, const int value) {
    EventData data;
    data[key] = EventDataEntry(value);
    return trigger(eventId, data);
}

Event *PluginManager::trigger(const String &eventId, const String &key, const float value) {
    EventData data;
    data[key] = EventDataEntry(value);
    return trigger(eventId, data);
}

void PluginManager::trigger(Event &event) {
    if (listeners.count(event.id)) {
        for (auto &callback : listeners[event.id]) {
            callback(event);
            if (event.stopPropagation) {
                break;
            }
        }
    }
}