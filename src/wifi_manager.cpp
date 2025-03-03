#include <WiFi.h>
#include "logger.h"
#include "led_service.h"

#define WIFI_SSID "ESP32Network"  
#define WIFI_PASS "MySecretPassword"  
#define WIFI_TIMEOUT_MS 20000     
#define WIFI_RETRY_DELAY_MS 1000   

// ✅ Define a static IP configuration
IPAddress local_IP(192, 168, 4, 10);  // Fixed IP for ESP32
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

void wifi_init() {
    LOG("🔍 Connecting to WiFi...");
    led_set_flashing(500);  

    WiFi.disconnect(true, true);  // Forget old connection
    delay(1000);

    WiFi.mode(WIFI_STA);

    // ✅ Apply static IP configuration before connecting
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        LOG("❌ Failed to configure static IP!");
    }

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED) {
        LOGF("WiFi Status: %d", WiFi.status());

        if (WiFi.status() == WL_DISCONNECTED) {
            LOG("🔄 Disconnected! Restarting Wi-Fi...");
            WiFi.disconnect(true, true);
            delay(1000);
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            startAttemptTime = millis();
        }

        if (millis() - startAttemptTime > WIFI_TIMEOUT_MS) {
            LOG("\n❌ WiFi connection timeout! Restarting...");
            ESP.restart();
        }

        delay(WIFI_RETRY_DELAY_MS);
        LOG(".");
    }

    LOG("\n✅ Connected! IP Address: ");
    LOG(WiFi.localIP());

    led_set_solid(5000);  
}
