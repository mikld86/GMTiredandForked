#ifndef PROCESS_H
#define PROCESS_H

#include "constants.h"
#include <queue>

constexpr int PREDICTIVE_MEASUREMENTS = 30; // 3s
constexpr int PREDICTIVE_TIME_MS = 1000;

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
    int brewTime;
    int brewVolume;
    int brewPressurize = PRESSURIZE_TIME;
    unsigned long currentPhaseStarted = 0;
    double currentVolume = 0;
    std::queue<double> measurements;

    explicit BrewProcess(ProcessTarget target = ProcessTarget::TIME, int infusionPumpTime = 0, int infusionBloomTime = 0,
                         int brewTime = 0, int brewVolume = 0)
        : target(target), infusionPumpTime(infusionPumpTime), infusionBloomTime(infusionBloomTime), brewTime(brewTime),
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
            return brewTime;
        default:
            return 0;
        }
    }

    double volumePerSecond() const {
        double sum = 0.0;
        for (auto p: measurements) {
            sum += p;
        }
        return sum / measurements.size() * (1000.0 / PROGRESS_INTERVAL);
    }

    bool isCurrentPhaseFinished() const {
        if (phase == BrewPhase::BREW_PUMP && target == ProcessTarget::VOLUMETRIC) {
            if (millis() - currentPhaseStarted > BREW_SAFETY_DURATION_MS) {
                return true;
            }
            const double predictiveFactor = volumePerSecond() / 1000.0 * PREDICTIVE_TIME_MS;
            return currentVolume + predictiveFactor >= brewVolume;
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
        // Progress should be called around every 100ms, as defined in PROGRESS_INTERVAL
        measurements.push(currentVolume);
        while (measurements.size() > PREDICTIVE_MEASUREMENTS) {
            measurements.pop();
        }

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
    std::queue<double> measurements;

    double currentVolume = 0;

    explicit GrindProcess(ProcessTarget target = ProcessTarget::TIME, int time = 0, int volume = 0)
        : target(target), time(time), volume(volume) {
        started = millis();
    }

    double volumePerSecond() const {
        double sum = 0.0;
        for (auto p: measurements) {
            sum += p;
        }
        return sum / measurements.size() * (1000.0 / PROGRESS_INTERVAL);
    }

    void updateVolume(double volume) override { currentVolume = volume; };

    bool isRelayActive() override { return false; }

    bool isAltRelayActive() override { return isActive(); }

    float getPumpValue() override { return 0.f; }

    void progress() override {
        // Progress should be called around every 100ms, as defined in PROGRESS_INTERVAL
        measurements.push(currentVolume);
        while (measurements.size() > PREDICTIVE_MEASUREMENTS) {
            measurements.pop();
        }
    }

    bool isActive() override {
        if (target == ProcessTarget::TIME) {
            return millis() - started < time;
        }
        const double predictiveFactor = volumePerSecond() / 1000.0 * PREDICTIVE_TIME_MS;
        return currentVolume + predictiveFactor < volume;
    }

    int getType() override { return MODE_GRIND; }
};

#endif // PROCESS_H
