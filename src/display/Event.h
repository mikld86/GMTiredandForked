#ifndef EVENT_H
#define EVENT_H

#include <Arduino.h>
#include <map>
#include <utility>

enum EventType {
    EVENT_TYPE_INT,
    EVENT_TYPE_FLOAT,
    EVENT_TYPE_STRING,
    EVENT_TYPE_NONE
};

struct EventDataEntry {
    EventType type;
    int intValue;
    float floatValue;
    String stringValue;

    EventDataEntry() : type(EVENT_TYPE_NONE), intValue(0), floatValue(0.0f), stringValue("") {}

    explicit EventDataEntry(int value) : type(EVENT_TYPE_INT), intValue(value), floatValue(0.0f), stringValue("") {}
    explicit EventDataEntry(float value) : type(EVENT_TYPE_FLOAT), intValue(0), floatValue(value), stringValue("") {}
    explicit EventDataEntry(String value) : type(EVENT_TYPE_STRING), intValue(0), floatValue(0.0f), stringValue(value) {}

    int asInt() const { return (type == EVENT_TYPE_INT) ? intValue : 0; }
    float asFloat() const { return (type == EVENT_TYPE_FLOAT) ? floatValue : 0.0f; }
    String asString() const { return (type == EVENT_TYPE_STRING) ? stringValue : ""; }
};

using EventData = std::map<String, EventDataEntry>;

struct Event {
    String id;
    EventData data;
    bool stopPropagation = false;

    void setInt(const String& key, int value) {
        data[key] = EventDataEntry(value);
    }

    void setFloat(const String& key, float value) {
        data[key] = EventDataEntry(value);
    }

    void setString(const String& key, const String& value) {
        data[key] = EventDataEntry(value);
    }

    int getInt(const String& key) const {
        return data.at(key).asInt();
    }

    float getFloat(const String& key) const {
        return data.at(key).asFloat();
    }

    String getString(const String& key) const {
        return data.at(key).asString();
    }
};

#endif // EVENT_H