#ifndef SETTINGS_H
#define SETTINGS_H

#include "constants.h"

#include <Arduino.h>
#include <Preferences.h>

#define PREFERENCES_KEY "controller"

class Settings;
using SettingsCallback = std::function<void(Settings *)>;

class Settings {
  public:
    Settings();

    void batchUpdate(const SettingsCallback &callback);
    void save();

    // Getters and setters
    int getTargetBrewTemp() const { return targetBrewTemp; }
    int getTargetSteamTemp() const { return targetSteamTemp; }
    int getTargetWaterTemp() const { return targetWaterTemp; }
    int getTemperatureOffset() const { return temperatureOffset; }
    int getTargetDuration() const { return targetDuration; }
    int getTargetVolume() const { return targetVolume; }
    int getTargetGrindVolume() const { return targetGrindVolume; }
    int getTargetGrindDuration() const { return targetGrindDuration; }
    int getStartupMode() const { return startupMode; }
    int getInfuseBloomTime() const { return infuseBloomTime; }
    int getInfusePumpTime() const { return infusePumpTime; }
    String getPid() const { return pid; }
    String getWifiSsid() const { return wifiSsid; }
    String getWifiPassword() const { return wifiPassword; }
    String getMdnsName() const { return mdnsName; }
    bool isHomekit() const { return homekit; }
    bool isVolumetricTarget() const { return volumetricTarget; }
    String getOTAChannel() const { return otaChannel; }
    String getSavedScale() const { return savedScale; }
    bool isBoilerFillActive() const { return boilerFillActive; }
    int getStartupFillTime() const { return startupFillTime; }
    int getSteamFillTime() const { return steamFillTime; }
    void setTargetBrewTemp(int target_brew_temp);
    void setTargetSteamTemp(int target_steam_temp);
    void setTargetWaterTemp(int target_water_temp);
    void setTemperatureOffset(int temperature_offset);
    void setTargetDuration(int target_duration);
    void setTargetVolume(int target_volume);
    void setTargetGrindVolume(int target_grind_volume);
    void setTargetGrindDuration(int target_duration);
    void setStartupMode(int startup_mode);
    void setInfuseBloomTime(int infuse_bloom_time);
    void setInfusePumpTime(int infuse_pump_time);
    void setPid(const String &pid);
    void setWifiSsid(const String &wifiSsid);
    void setWifiPassword(const String &wifiPassword);
    void setMdnsName(const String &mdnsName);
    void setHomekit(bool homekit);
    void setVolumetricTarget(bool volumetric_target);
    void setOTAChannel(const String &otaChannel);
    void setSavedScale(const String &savedScale);
    void setBoilerFillActive(bool boiler_fill_active);
    void setStartupFillTime(int startup_fill_time);
    void setSteamFillTime(int steam_fill_time);

  private:
    Preferences preferences;
    bool inBatch = false;

    int targetBrewTemp = 93;
    int targetSteamTemp = 155;
    int targetWaterTemp = 80;
    int temperatureOffset = DEFAULT_TEMPERATURE_OFFSET;
    int targetDuration = 25000;
    int targetVolume = 36;
    int targetGrindVolume = 18;
    int targetGrindDuration = 25000;
    int startupMode = MODE_STANDBY;
    int infuseBloomTime = 0;
    int infusePumpTime = 0;
    String pid = DEFAULT_PID;
    String wifiSsid = "";
    String wifiPassword = "";
    String mdnsName = DEFAULT_MDNS_NAME;
    String savedScale = "";
    bool homekit = false;
    bool volumetricTarget = false;
    bool boilerFillActive = false;
    int startupFillTime = 0;
    int steamFillTime = 0;

    String otaChannel = DEFAULT_OTA_CHANNEL;
};

#endif // SETTINGS_H
