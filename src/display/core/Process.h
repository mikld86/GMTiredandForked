#ifndef PROCESS_H
#define PROCESS_H

#include "constants.h"

class Process {
  public:
    Process() = default;
    virtual ~Process() = default;

    virtual bool isRelayActive() = 0;

    virtual bool isAltRelayActive() = 0;

    virtual float getPumpValue() = 0;

    virtual void progress() = 0;

    virtual bool isActive() = 0;

    virtual int getType() = 0;

    virtual void updateVolume(double volume) = 0;
};

enum class BrewPhase { INFUSION_PRESSURIZE, INFUSION_PUMP, INFUSION_BLOOM, BREW_PRESSURIZE, BREW_PUMP, FINISHED };

enum class ProcessTarget { TIME, VOLUMETRIC };

class BrewProcess : public Process {
  public:
    BrewPhase phase = BrewPhase::INFUSION_PRESSURIZE;
    ProcessTarget target;
    int infusionPumpTime;
    int infusionBloomTime;
    int brewSeconds;
    int brewVolume;
    int brewPressurize = PRESSURIZE_TIME;

    unsigned long currentPhaseStarted = 0;
    double currentVolume = 0;

    explicit BrewProcess(ProcessTarget target = ProcessTarget::TIME, int infusionPumpTime = 0, int infusionBloomTime = 0,
                         int brewSeconds = 0, int brewVolume = 0)
        : target(target), infusionPumpTime(infusionPumpTime), infusionBloomTime(infusionBloomTime), brewSeconds(brewSeconds),
          brewVolume(brewVolume) {
        if (infusionBloomTime == 0 || infusionPumpTime == 0) {
            phase = BrewPhase::BREW_PRESSURIZE;
        }
        currentPhaseStarted = millis();
    }

    void updateVolume(double volume) override { currentVolume = volume; };

    unsigned long getPhaseDuration() const {
        switch (phase) {
        case BrewPhase::INFUSION_PRESSURIZE:
            return brewPressurize;
        case BrewPhase::INFUSION_PUMP:
            return infusionPumpTime;
        case BrewPhase::INFUSION_BLOOM:
            return infusionBloomTime;
        case BrewPhase::BREW_PRESSURIZE:
            return brewPressurize;
        case BrewPhase::BREW_PUMP:
            return brewSeconds;
        default:
            return 0;
        }
    }

    bool isCurrentPhaseFinished() const {
        if (phase == BrewPhase::BREW_PUMP && target == ProcessTarget::VOLUMETRIC) {
            if (millis() - currentPhaseStarted > BREW_SAFETY_DURATION_MS) {
                return true;
            }
            return currentVolume >= brewVolume;
        }
        if (phase != BrewPhase::FINISHED) {
            return millis() - currentPhaseStarted > getPhaseDuration();
        }
        return true;
    }

    bool isRelayActive() override {
        return phase == BrewPhase::INFUSION_PUMP || phase == BrewPhase::INFUSION_BLOOM || phase == BrewPhase::BREW_PUMP;
    }

    bool isAltRelayActive() override { return false; }

    float getPumpValue() override {
        if (phase == BrewPhase::INFUSION_PRESSURIZE || phase == BrewPhase::INFUSION_PUMP || phase == BrewPhase::BREW_PRESSURIZE ||
            phase == BrewPhase::BREW_PUMP) {
            return 100.f;
        }
        return 0.f;
    }

    void progress() override {
        if (isCurrentPhaseFinished()) {
            currentPhaseStarted = millis();
            switch (phase) {
            case BrewPhase::INFUSION_PRESSURIZE:
                phase = BrewPhase::INFUSION_PUMP;
                break;
            case BrewPhase::INFUSION_PUMP:
                phase = BrewPhase::INFUSION_BLOOM;
                break;
            case BrewPhase::INFUSION_BLOOM:
                phase = BrewPhase::BREW_PRESSURIZE;
                break;
            case BrewPhase::BREW_PRESSURIZE:
                phase = BrewPhase::BREW_PUMP;
                break;
            case BrewPhase::BREW_PUMP:
                phase = BrewPhase::FINISHED;
                break;
            default:;
            }
        }
    }

    bool isActive() override { return phase != BrewPhase::FINISHED; }

    int getType() override { return MODE_BREW; }
};

class SteamProcess : public Process {
  public:
    float pumpValue = 4.f;
    int duration;
    unsigned long started;

    explicit SteamProcess(int duration = STEAM_SAFETY_DURATION_MS, float pumpValue = 4.f)
        : pumpValue(pumpValue), duration(duration) {
        started = millis();
    }

    bool isRelayActive() override { return false; };

    bool isAltRelayActive() override { return false; };

    float getPumpValue() override { return isActive() ? pumpValue : 0.f; };

    void progress() override {
        // Stateless implmentation
    };

    bool isActive() override {
        unsigned long now = millis();
        return now - started < duration;
    };

    int getType() override { return MODE_STEAM; }

    void updateVolume(double volume) override {};
};

class PumpProcess : public Process {
  public:
    int duration;
    unsigned long started;

    explicit PumpProcess(int duration = HOT_WATER_SAFETY_DURATION_MS) : duration(duration) { started = millis(); }

    bool isRelayActive() override { return false; };

    bool isAltRelayActive() override { return false; };

    float getPumpValue() override { return isActive() ? 100.f : 0.f; };

    void progress() override {
        // Stateless implementation
    };

    bool isActive() override {
        unsigned long now = millis();
        return now - started < duration;
    };

    int getType() override { return MODE_WATER; }

    void updateVolume(double volume) override {};
};

class GrindProcess : public Process {
  public:
    ProcessTarget target;
    int time;
    int volume;
    unsigned long started;

    double currentVolume = 0;

    explicit GrindProcess(ProcessTarget target = ProcessTarget::TIME, int time = 0, int volume = 0)
        : target(target), time(time), volume(volume) {
        started = millis();
    }

    void updateVolume(double volume) override { currentVolume = volume; };

    bool isRelayActive() override { return false; }

    bool isAltRelayActive() override { return isActive(); }

    float getPumpValue() override { return 0.f; }

    void progress() override {
        // Stateless implementation
    }

    bool isActive() override { return target == ProcessTarget::TIME ? millis() - started < time : currentVolume < volume; }

    int getType() override { return MODE_GRIND; }
};

#endif // PROCESS_H
