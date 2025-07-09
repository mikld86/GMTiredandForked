#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <functional>

class WiFiManager {
public:
    // Callback function types
    using ConnectCallback = std::function<void(void)>;
    using DisconnectCallback = std::function<void(void)>;
    using APStartCallback = std::function<void(void)>;
    using APStopCallback = std::function<void(void)>;

    struct WiFiConfig {
        String ssid;
        String password;
        String apSSID;
        String apPassword;
        uint32_t apTimeoutMs;

        WiFiConfig(const char* wifi_ssid = "",
                  const char* wifi_password = "",
                  const char* ap_ssid = "GaggiMate",
                  const char* ap_password = "",
                  uint32_t ap_timeout_ms = 5 * 60 * 1000)  // 5 minutes default
            : ssid(wifi_ssid)
            , password(wifi_password)
            , apSSID(ap_ssid)
            , apPassword(ap_password)
            , apTimeoutMs(ap_timeout_ms) {}
    };

private:
    // Configuration
    static const uint8_t MAX_CONNECTION_ATTEMPTS = 10;
    static const uint32_t RECONNECT_DELAY_MS = 5000;  // 5 seconds between reconnection attempts

    // State variables
    WiFiConfig config;
    bool isAPActive = false;
    uint32_t apStartTime = 0;
    TaskHandle_t wifiTaskHandle = nullptr;
    bool shouldReconnect = false;

    // Callbacks
    ConnectCallback onConnectCallback = nullptr;
    DisconnectCallback onDisconnectCallback = nullptr;
    APStartCallback onAPStartCallback = nullptr;
    APStopCallback onAPStopCallback = nullptr;

    // Event group for WiFi events
    EventGroupHandle_t wifiEventGroup;
    static const int WIFI_CONNECTED_BIT = BIT0;
    static const int WIFI_FAIL_BIT = BIT1;
    static const int WIFI_RECONNECT_BIT = BIT2;


    bool hasCredentials() const {
        return !config.ssid.isEmpty() && !config.password.isEmpty();
    }

    void startConnection() {
        if (hasCredentials()) {
            Serial.printf("Attempting to connect to %s\n", config.ssid.c_str());
            WiFi.begin(config.ssid.c_str(), config.password.c_str());
        } else {
            Serial.println("No credentials stored - starting AP mode");
            xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
        }
    }


    static void wifiEventHandler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
        WiFiManager* manager = static_cast<WiFiManager*>(arg);

        if (event_base == WIFI_EVENT) {
            switch (event_id) {
                case WIFI_EVENT_STA_DISCONNECTED: {
                    wifi_event_sta_disconnected_t* event =
                        (wifi_event_sta_disconnected_t*) event_data;
                    Serial.printf("WiFi disconnected. Reason: %d\n", event->reason);
                    xEventGroupClearBits(manager->wifiEventGroup, WIFI_CONNECTED_BIT);
                    xEventGroupSetBits(manager->wifiEventGroup, WIFI_FAIL_BIT);
                    if (manager->onDisconnectCallback) {
                        manager->onDisconnectCallback();
                    }
                    break;
                }
            }
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            Serial.printf("Got IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(manager->wifiEventGroup, WIFI_CONNECTED_BIT);
            xEventGroupClearBits(manager->wifiEventGroup, WIFI_FAIL_BIT);
            if (manager->onConnectCallback) {
                manager->onConnectCallback();
            }
        }
    }

    static void wifiTask(void* parameter) {
        WiFiManager* manager = static_cast<WiFiManager*>(parameter);
        uint8_t attempts = 0;
        TickType_t lastConnectionAttempt = 0;

        // If no credentials, start AP immediately
        if (!manager->hasCredentials()) {
            manager->startAP();
        }

        while (true) {
            // Check if we need to reconnect with new credentials
            if (xEventGroupGetBits(manager->wifiEventGroup) & WIFI_RECONNECT_BIT) {
                WiFi.disconnect(true);
                vTaskDelay(pdMS_TO_TICKS(1000));
                manager->startConnection();
                attempts = 0;
                lastConnectionAttempt = xTaskGetTickCount();
                xEventGroupClearBits(manager->wifiEventGroup, WIFI_RECONNECT_BIT);
            }

            // Always try to connect if we have credentials and aren't connected
            if (manager->hasCredentials() && WiFi.status() != WL_CONNECTED) {
                // Check if it's time for next connection attempt
                if (xTaskGetTickCount() - lastConnectionAttempt >= pdMS_TO_TICKS(RECONNECT_DELAY_MS)) {
                    Serial.printf("Attempting to connect to %s (Attempt %d)\n",
                                manager->config.ssid.c_str(), attempts + 1);

                    // Start AP after certain number of failures if not already active
                    if (attempts >= MAX_CONNECTION_ATTEMPTS && !manager->isAPActive) {
                        manager->startAP();
                    }

                    // Try to connect regardless of AP state
                    esp_wifi_connect();
                    attempts++;
                    lastConnectionAttempt = xTaskGetTickCount();
                }
            }

            // Handle connection state
            if (WiFi.status() == WL_CONNECTED) {
                if (!xEventGroupGetBits(manager->wifiEventGroup) & WIFI_CONNECTED_BIT) {
                    // Just connected
                    xEventGroupSetBits(manager->wifiEventGroup, WIFI_CONNECTED_BIT);
                    xEventGroupClearBits(manager->wifiEventGroup, WIFI_FAIL_BIT);
                    attempts = 0;
                    if (manager->onConnectCallback) {
                        manager->onConnectCallback();
                    }
                }

                // If connected and AP is active, check timeout
                if (manager->isAPActive &&
                    manager->config.apTimeoutMs > 0 &&
                    (millis() - manager->apStartTime >= manager->config.apTimeoutMs)) {
                    manager->stopAP();
                }
            } else {
                // Not connected
                if (xEventGroupGetBits(manager->wifiEventGroup) & WIFI_CONNECTED_BIT) {
                    // Just disconnected
                    xEventGroupClearBits(manager->wifiEventGroup, WIFI_CONNECTED_BIT);
                    xEventGroupSetBits(manager->wifiEventGroup, WIFI_FAIL_BIT);
                    if (manager->onDisconnectCallback) {
                        manager->onDisconnectCallback();
                    }
                }
            }

            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }


public:
    WiFiManager(const WiFiConfig& initial_config = WiFiConfig())
        : config(initial_config) {
        wifiEventGroup = xEventGroupCreate();
    }

    void begin() {
        // Initialize WiFi
        WiFi.mode(WIFI_MODE_APSTA);
        WiFi.disconnect(true);

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;

        esp_event_handler_instance_register(WIFI_EVENT,
                                          ESP_EVENT_ANY_ID,
                                          &wifiEventHandler,
                                          this,
                                          &instance_any_id);
        esp_event_handler_instance_register(IP_EVENT,
                                          IP_EVENT_STA_GOT_IP,
                                          &wifiEventHandler,
                                          this,
                                          &instance_got_ip);

        if (hasCredentials()) {
            startConnection();
        }

        // Create WiFi management task
        xTaskCreate(wifiTask, "WiFiManager", 4096, this, 1, &wifiTaskHandle);
    }

    // Callback setters
    void setOnConnectCallback(ConnectCallback callback) {
        onConnectCallback = callback;
    }

    void setOnDisconnectCallback(DisconnectCallback callback) {
        onDisconnectCallback = callback;
    }

    void setOnAPStartCallback(APStartCallback callback) {
        onAPStartCallback = callback;
    }

    void setOnAPStopCallback(APStopCallback callback) {
        onAPStopCallback = callback;
    }

    // Configuration methods
    void reconfigure(const WiFiConfig& new_config) {
        config = new_config;
        xEventGroupSetBits(wifiEventGroup, WIFI_RECONNECT_BIT);
    }

    void updateCredentials(const char* new_ssid, const char* new_password) {
        config.ssid = new_ssid;
        config.password = new_password;
        xEventGroupSetBits(wifiEventGroup, WIFI_RECONNECT_BIT);
    }

    void updateAPConfig(const char* ap_ssid, const char* ap_password, uint32_t timeout_ms) {
        config.apSSID = ap_ssid;
        config.apPassword = ap_password;
        config.apTimeoutMs = timeout_ms;

        // If AP is active, restart it with new configuration
        if (isAPActive) {
            stopAP();
            startAP();
        }
    }

    void startAP() {
        if (!isAPActive) {
            Serial.println("Starting AP mode");
            WiFi.softAP(config.apSSID.c_str(), config.apPassword.c_str());
            isAPActive = true;
            apStartTime = millis();
            Serial.printf("AP '%s' started. Will timeout in %d seconds\n",
                        config.apSSID.c_str(), config.apTimeoutMs / 1000);

            if (onAPStartCallback) {
                onAPStartCallback();
            }
        }
    }

    void stopAP() {
        if (isAPActive) {
            Serial.println("Stopping AP mode");
            WiFi.softAPdisconnect(true);
            isAPActive = false;

            if (onAPStopCallback) {
                onAPStopCallback();
            }
        }
    }

    bool isConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

    bool isAccessPointActive() {
        return isAPActive;
    }

    String getLocalIP() {
        return WiFi.localIP().toString();
    }

    String getAPIP() {
        return WiFi.softAPIP().toString();
    }

    const WiFiConfig& getCurrentConfig() const {
        return config;
    }
};

#endif
