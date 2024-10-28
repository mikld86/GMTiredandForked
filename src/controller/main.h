#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include "PID_v1.h"
#include "PID_AutoTune_v0.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SPI.h>
#include "NimBLEServerController.h"

// GPIO Pin Definitions
#define HEATER_PIN       SDA
#define PUMP_PIN         SCL
#define VALVE_PIN        MOSI
#define MAX6675_SCK_PIN  SCK
#define MAX6675_CS_PIN   SS
#define MAX6675_MISO_PIN MISO

#define PWM_FREQUENCY 20

#define I2C_DEV_ADDR 0x55

#define PUMP_CYCLE_TIME 5000

// Function prototypes for initialization
void setup(void);
void loop(void);

// Function Prototypes
void control_heater(int signal);
void control_pump();
void control_valve(bool state);
float read_temperature(void);
void on_temperature_control(float temperature);
void on_pump_control(uint8_t setpoint);
void on_valve_control(bool state);
void on_ping();
void on_autotune();
void on_error(int error_code);
void handle_ping_timeout(void);
void thermal_runaway_shutdown(void);
void start_pid_autotune(void);
void stop_pid_autotune(void);

#endif //MAIN_H
