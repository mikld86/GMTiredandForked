#include "main.h"
#include <lvgl.h>

Controller controller;

void setup() {
    Serial.begin(115200);
    controller.setup();
}

void loop() {
    if (!controller.isUpdating()) {
        lv_timer_handler();
    }
    controller.loop();
    delay(2);
}
