#include <WiFi.h>
#include "logger.h"
#include "led_service.h"

#define WIFI_SSID "ESP32Network"   // Change to your SSID
#define WIFI_PASS "MySecretPassword"               // Leave empty for open network
#define WIFI_TIMEOUT_MS 20000      // Max time to wait for WiFi (20 seconds)
#define WIFI_RETRY_DELAY_MS 1000   // Time between connection attempts (1 second)

void wifi_init() {
    LOG("🔍 Connecting to WiFi...");
    led_set_flashing(500);  // ✅ Flash LED while connecting

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttemptTime > WIFI_TIMEOUT_MS) {
            LOG("\n❌ WiFi connection failed! Restarting...");
            led_set_solid(2000);  // ✅ Solid LED for 2 sec (non-blocking)
            ESP.restart();
        }
        delay(WIFI_RETRY_DELAY_MS);
        LOG(".");
    }

    LOG("\n✅ Connected! IP Address: ");
    LOG(WiFi.localIP());

    led_set_solid(5000);  // ✅ Solid LED for 5 sec, then turns off (non-blocking)
}
