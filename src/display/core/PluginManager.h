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
    void registerPlugin(Plugin *plugin);

    void setup(Controller *controller);
    void loop();

    void on(const String &eventId, const EventCallback &callback);

    Event *trigger(const String &eventId, const EventData &data);
    Event *trigger(const String &eventId, const String &key, const String &value);
    Event *trigger(const String &eventId, const String &key, const int value);
    Event *trigger(const String &eventId, const String &key, const float value);
    void trigger(Event &event);

  private:
    bool initialized = false;
    std::vector<Plugin *> plugins;
    std::map<String, std::vector<EventCallback>> listeners;
};

#endif // PLUGINMANAGER_H
