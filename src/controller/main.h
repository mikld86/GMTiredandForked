#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SPI.h>
#include "NimBLEServerController.h"

// GPIO Pin Definitions
#define HEATER_PIN       14
#define PUMP_PIN         9
#define VALVE_PIN        10
#define MAX6675_SCK_PIN  6
#define MAX6675_CS_PIN   7
#define MAX6675_MISO_PIN 4

#define PWM_FREQUENCY 20

#define TEMP_UPDATE_INTERVAL_MS 1000

#define I2C_DEV_ADDR 0x55

#define PUMP_CYCLE_TIME 5000

#define RELAY_ON HIGH

// Function prototypes for initialization
void setup(void);
void loop(void);

// Function Prototypes
void control_heater(int signal);
void control_pump();
void control_valve(bool state);
float read_temperature(void);
void on_temperature_control(float temperature);
void on_pump_control(float setpoint);
void on_valve_control(bool state);
void on_pid_control(float Kp, float Ki, float Kd);
void on_ping();
void on_autotune();
void on_error(int error_code);
void handle_ping_timeout(void);
void thermal_runaway_shutdown(void);
void start_pid_autotune(void);
void stop_pid_autotune(void);

#endif //MAIN_H
