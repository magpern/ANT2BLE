#ifndef LED_SERVICE_H
#define LED_SERVICE_H

#include <Arduino.h>
#include <Ticker.h>

#ifndef LED_PIN
    #error "LED_PIN is not defined! Set it in platformio.ini using build_flags."
#endif

void led_init();
void led_set_solid(int duration_ms);
void led_set_flashing(int interval_ms);
void led_turn_off();

#endif
