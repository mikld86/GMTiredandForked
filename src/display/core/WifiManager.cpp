// WifiManager.cpp — v1.6.1
#include "WifiManager.h"

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "Arduino.h"
#include "WiFi.h"

// (Optional: uncomment if you want to stop IPv6 link-local on STA to avoid fe80:: traffic)
// #include "esp_netif.h"

bool WifiManager::hasCredentials() const { return !config.ssid.isEmpty() && !config.password.isEmpty(); }

void WifiManager::startConnection() const {
    if (hasCredentials()) {
        ESP_LOGI("WifiManager", "Attempting to connect to %s", config.ssid.c_str());
        // Ensure DHCP client will request a fresh lease on this join
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.begin(config.ssid.c_str(), config.password.c_str());
    } else {
        ESP_LOGI("WifiManager", "No credentials stored - starting AP mode");
        xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
    }
}

void WifiManager::wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    WifiManager *manager = static_cast<WifiManager *>(arg);

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_DISCONNECTED: {
            auto *event = (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGI("WifiManager", "WiFi disconnected. Reason: %d", event ? event->reason : -1);
            xEventGroupClearBits(manager->wifiEventGroup, WIFI_CONNECTED_BIT);
            xEventGroupSetBits(manager->wifiEventGroup, WIFI_FAIL_BIT);
            manager->pluginManager->trigger("controller:wifi:disconnect");
            break;
        }
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto *event = (ip_event_got_ip_t *)event_data;
        if (event) {
            ESP_LOGI("WifiManager", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        }
        xEventGroupSetBits(manager->wifiEventGroup, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(manager->wifiEventGroup, WIFI_FAIL_BIT);
        manager->pluginManager->trigger("controller:wifi:connect", "AP", 0);
        if (manager->isAPActive) {
            manager->stopAP();
        }
    }
}

void WifiManager::wifiTask(void *parameter) {
    WifiManager *manager = static_cast<WifiManager *>(parameter);
    uint8_t attempts = 0;
    TickType_t lastConnectionAttempt = 0;

    // If no credentials, start AP immediately
    if (!manager->hasCredentials()) {
        manager->startAP();
    }

    while (true) {
        // Check if we need to reconnect with new credentials
        if (xEventGroupGetBits(manager->wifiEventGroup) & WIFI_RECONNECT_BIT) {
            // Keep radio powered; force fresh DHCP; reconnect deterministically
            WiFi.disconnect(false);
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
            vTaskDelay(pdMS_TO_TICKS(200));
            manager->startConnection();
            attempts = 0;
            lastConnectionAttempt = xTaskGetTickCount();
            xEventGroupClearBits(manager->wifiEventGroup, WIFI_RECONNECT_BIT);
        }

        // Always try to connect if we have credentials and aren't connected
        if (manager->hasCredentials() && WiFi.status() != WL_CONNECTED) {
            // Check if it's time for next connection attempt
            if (xTaskGetTickCount() - lastConnectionAttempt >= pdMS_TO_TICKS(RECONNECT_DELAY_MS)) {
                ESP_LOGI("WifiManager", "Attempting to connect to %s (Attempt %u)",
                         manager->config.ssid.c_str(), (unsigned)attempts + 1);

                // Start AP after certain number of failures if not already active
                if (attempts >= MAX_CONNECTION_ATTEMPTS && !manager->isAPActive) {
                    manager->startAP();
                }

                // Try to connect regardless of AP state — always arm DHCP and reload creds
                WiFi.setTxPower(WIFI_POWER_19_5dBm);
                WiFi.disconnect(false);
                WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
                WiFi.begin(manager->config.ssid.c_str(), manager->config.password.c_str());

                attempts++;
                lastConnectionAttempt = xTaskGetTickCount();
            }
        }

        // Handle connection state
        if (WiFi.status() == WL_CONNECTED) {
            // FIX: operator precedence — run this once on "just connected"
            if ( !(xEventGroupGetBits(manager->wifiEventGroup) & WIFI_CONNECTED_BIT) ) {
                xEventGroupSetBits(manager->wifiEventGroup, WIFI_CONNECTED_BIT);
                xEventGroupClearBits(manager->wifiEventGroup, WIFI_FAIL_BIT);
                attempts = 0;
                manager->pluginManager->trigger("controller:wifi:connect", "AP", manager->isAPActive ? 1 : 0);
            }

            // If connected and AP is active, check timeout
            if (manager->isAPActive && manager->config.apTimeoutMs > 0 &&
                (millis() - manager->apStartTime >= manager->config.apTimeoutMs)) {
                manager->stopAP();
            }
        } else {
            // Not connected
            if (xEventGroupGetBits(manager->wifiEventGroup) & WIFI_CONNECTED_BIT) {
                // Just disconnected
                xEventGroupClearBits(manager->wifiEventGroup, WIFI_CONNECTED_BIT);
                xEventGroupSetBits(manager->wifiEventGroup, WIFI_FAIL_BIT);
                manager->pluginManager->trigger("controller:wifi:disconnect");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void WifiManager::begin() {
    // Initialize WiFi — keep APSTA as in 1.6, but avoid hard power-downs
    WiFi.mode(WIFI_MODE_APSTA);

    // Keep radio powered; we’ll reload creds & DHCP before each connect
    WiFi.disconnect(false);

    // Don’t doze during DHCP OFFER→REQUEST timing
    WiFi.setSleep(false);

    // (Optional but helpful) make behavior explicit
    WiFi.setAutoReconnect(true);
    // WiFi.setHostname("GaggiMate"); // uncomment to set a stable DHCP hostname

    // (Optional: disable IPv6 on STA to avoid fe80:: traffic if your firewall blocks IPv6)
    // if (auto sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF")) {
    //     esp_netif_ip6_stop(sta);
    // }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, this, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, this, &instance_got_ip);

    if (hasCredentials()) {
        startConnection();
    }

    // Create WiFi management task
    xTaskCreate(wifiTask, "WifiManager", 4096, this, 1, &wifiTaskHandle);
}

void WifiManager::reconfigure(const WiFiConfig &new_config) {
    config = new_config;
    xEventGroupSetBits(wifiEventGroup, WIFI_RECONNECT_BIT);
}

void WifiManager::updateCredentials(const char *new_ssid, const char *new_password) {
    config.ssid = new_ssid;
    config.password = new_password;
    xEventGroupSetBits(wifiEventGroup, WIFI_RECONNECT_BIT);
}

void WifiManager::updateAPConfig(const char *ap_ssid, const char *ap_password, uint32_t timeout_ms) {
    config.apSSID = ap_ssid;
    config.apPassword = ap_password;
    config.apTimeoutMs = timeout_ms;

    // If AP is active, restart it with new configuration
    if (isAPActive) {
        stopAP();
        startAP();
    }
}

void WifiManager::startAP() {
    if (!isAPActive) {
        ESP_LOGI("WifiManager", "Starting AP mode");

        // Keep AP IP config (as in 1.5.1) if you rely on AP provisioning
        // WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_IP, WIFI_SUBNET_MASK);
        WiFi.softAP(config.apSSID.c_str(), config.apPassword.c_str());
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        isAPActive = true;
        apStartTime = millis();
        ESP_LOGI("WifiManager", "AP '%s' started. Will timeout in %d seconds",
                 config.apSSID.c_str(), config.apTimeoutMs / 1000);

        pluginManager->trigger("controller:wifi:connect", "AP", 1);
    }
}

void WifiManager::stopAP() {
    if (isAPActive) {
        ESP_LOGI("WifiManager", "Stopping AP mode");
        WiFi.softAPdisconnect(true);
        isAPActive = false;
        pluginManager->trigger("controller:wifi:disconnect");
    }
}
