#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H
#include "Event.h"
#include "Plugin.h"

#include <functional>
#include <vector>

using EventCallback = std::function<void(Event &)>;

class Controller;
class PluginManager {
public:
    void registerPlugin(Plugin* plugin) {
        plugins.push_back(plugin);
    }

    void setup(Controller * controller) {
        for (auto& plugin : plugins) {
            plugin->setup(controller, this);
        }
    }

    void loop() {
        for (auto& plugin : plugins) {
            plugin->loop();
        }
    }

    void on(const String& eventId, const EventCallback &callback) {
        listeners[eventId].push_back(callback);
    }

    Event* trigger(const String& eventId, const EventData& data) {
        Event event;
        event.id = eventId;
        event.data = data;
        trigger(event);
        return &event;
    }

    Event* trigger(const String& eventId, const String& key, const String& value) {
        EventData data;
        data[key] = EventDataEntry(value);
        return trigger(eventId, data);
    }

    Event* trigger(const String& eventId, const String& key, const int value) {
        EventData data;
        data[key] = EventDataEntry(value);
        return trigger(eventId, data);
    }

    Event* trigger(const String& eventId, const String& key, const float value) {
        EventData data;
        data[key] = EventDataEntry(value);
        return trigger(eventId, data);
    }

    void trigger(Event& event) {
        if (listeners.count(event.id)) {
            for (auto& callback : listeners[event.id]) {
                callback(event);
                if (event.stopPropagation) {
                    break;
                }
            }
        }
    }

private:
    std::vector<Plugin*> plugins;
    std::map<String, std::vector<EventCallback>> listeners;
};

#endif //PLUGINMANAGER_H
