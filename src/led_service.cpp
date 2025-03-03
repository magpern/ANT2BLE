#include "led_service.h"

static Ticker ledTicker;  // ✅ ESP32 timer for handling blinking automatically
static int led_flash_interval = 0;
static unsigned long solid_led_end_time = 0;  // ✅ Track when solid LED should turn off
static bool led_state = false;

// ✅ Toggle LED for blinking mode
void toggle_led() {
    digitalWrite(LED_PIN, led_state);
    led_state = !led_state;
}

void led_init() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);  // Start with LED off
}

// ✅ Starts flashing the LED at the given interval (Autonomous)
void led_set_flashing(int interval_ms) {
    led_turn_off();  // Stop any solid LED mode
    led_flash_interval = interval_ms;
    ledTicker.attach_ms(led_flash_interval, toggle_led);  // ✅ Timer toggles LED autonomously
}

// ✅ Keeps LED solid for X milliseconds, then turns off automatically
void led_set_solid(int duration_ms) {
    led_turn_off();  // Ensure no previous blinking
    digitalWrite(LED_PIN, HIGH);
    solid_led_end_time = millis() + duration_ms;
}

// ✅ Turns off the LED completely
void led_turn_off() {
    ledTicker.detach();  // ✅ Stop blinking
    digitalWrite(LED_PIN, LOW);
    solid_led_end_time = 0;
}

// ✅ This runs in loop(), but it doesn't affect flashing LED mode
void led_update() {
    if (solid_led_end_time > 0 && millis() > solid_led_end_time) {
        led_turn_off();
    }
}
