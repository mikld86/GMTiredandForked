#ifndef ESP_GITHUB_OTA_H
#define ESP_GITHUB_OTA_H

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

#include "semver.h"

class GitHubOTA {
  public:
    GitHubOTA(String version, String release_url, String firmware_name = "firmware.bin",
              String filesystem_name = "filesystem.bin", bool fetch_url_via_redirect = false);

    void checkForUpdates();
    bool isUpdateAvailable() const;
    String getCurrentVersion() const;
    void update();

  private:
    HTTPUpdate Updater;

    HTTPUpdateResult update_filesystem(const String &url);
    HTTPUpdateResult update_firmware(const String &url);

    semver_t _version;
    semver_t _latest_version = {0, 0, 0, nullptr, nullptr};
    String _release_url;
    String _latest_url;
    String _firmware_name;
    String _filesystem_name;
    bool _fetch_url_via_redirect;
    WiFiClientSecure _wifi_client;
};

#endif
