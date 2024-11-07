#ifndef NIMBLECOMM_H
#define NIMBLECOMM_H

#include <Arduino.h>
#include <NimBLEDevice.h>

// UUIDs for BLE services and characteristics
#define SERVICE_UUID "e75bc5b6-ff6e-4337-9d31-0c128f2e6e68"
#define TEMP_CONTROL_CHAR_UUID  "fd668d9a-15d1-4968-aa29-deda8bb0e73a"
#define PUMP_CONTROL_CHAR_UUID  "1f0739ac-9638-4c4b-9665-5d7684d30249"
#define VALVE_CONTROL_CHAR_UUID "485ea9f8-3e3f-4811-a299-c8ba6d1f4b78"
#define TEMP_READ_CHAR_UUID     "56887a3f-23fe-4181-afa5-8dad4d92721b"
#define PING_CHAR_UUID          "9731755e-29ce-41a8-91d9-7a244f49859b"
#define ERROR_CHAR_UUID         "d6676ec7-820c-41de-820d-95620749003b"
#define AUTOTUNE_CHAR_UUID      "d54df381-69b6-4531-b1cc-dde7766bbaf4"
#define PID_CONTROL_CHAR_UUID   "d448c469-3e1d-4105-b5b8-75bf7d492fad"

#define ERROR_CODE_COMM_SEND 1
#define ERROR_CODE_COMM_RCV 2
#define ERROR_CODE_PROTO_ERR 3
#define ERROR_CODE_RUNAWAY 4
#define ERROR_CODE_TIMEOUT 5


typedef std::function<void (float temperature)> temp_read_callback_t;
typedef void (*temp_control_callback_t)(float setpoint);
typedef void (*pump_control_callback_t)(float setpoint);
typedef void (*pin_control_callback_t)(bool isActive);
typedef void (*pid_control_callback_t)(float Kp, float Ki, float Kd);
typedef void (*ping_callback_t)();
typedef void (*remote_err_callback_t)(int errorCode);
typedef void (*autotune_callback_t)();

#endif //NIMBLECOMM_H
