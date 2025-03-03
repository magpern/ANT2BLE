#include <Arduino.h>
#include "ant_parser.h"
#include "ble_ftms.h"
#include "logger.h"
#include "esp_system.h"
#include "esp_timer.h"  // ✅ ESP32 Software Timer API
#include "wifi_manager.h"
#include "websocket_manager.h"
#include "led_service.h"

#define SERIAL_BAUDRATE 115200
#define FTMS_UPDATE_INTERVAL_US (2000000)  // 2 seconds in microseconds

void checkForReboot();  // Function declaration
void sendFTMSUpdate(void* arg);  // Timer callback function
void onBLEConnect();  // Function to start sending data
void onBLEDisconnect();  // Function to stop sending data

ANTParser antParser;
BLEFTMS bleFTMS;
esp_timer_handle_t ftmsTimer;  // ✅ ESP32 Timer Handle
bool isBLEConnected = false;   // ✅ Track BLE connection status

void setup() {
    Serial.begin(SERIAL_BAUDRATE);  // ANT+ Data Input (Raspberry Pi -> ESP32)
    logger.begin(SERIAL_BAUDRATE, SERIAL_8N1, 9, 10);  // Debug Output via GPIO9/10

    LOG("ESP32-S3 ANT+ to BLE FTMS");

    wifi_init();  // ✅ Simple WiFi connection

    // ✅ Ensure WiFi is connected before starting WebSockets
    while (WiFi.status() != WL_CONNECTED) {
        LOG("⏳ Waiting for WiFi...");
        delay(1000);
    }

    LOG("🚀 Starting WebSocket Server...");
    startWebSocketServer();
    LOG("✅ WebSocket Server Started!");

    // Initialize BLE FTMS
    bleFTMS.begin();

    // Register BLE Callbacks
    bleFTMS.setConnectCallback(onBLEConnect);
    bleFTMS.setDisconnectCallback(onBLEDisconnect);

    // Print unique ESP32-S3 MAC address
    LOG("Device BLE MAC: " + bleFTMS.getDeviceMAC());

    // ✅ Set up FTMS update timer (but don't start it yet)
    const esp_timer_create_args_t timerArgs = {
        .callback = &sendFTMSUpdate,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "FTMS Update Timer"
    };
    esp_timer_create(&timerArgs, &ftmsTimer);
}

void loop() {
    static unsigned long lastReconnectAttempt = 0;  // Track last reconnect time
    antParser.readSerial();
    checkForReboot();  // Check if "reboot" command is received

    // ✅ Restart advertising if it stops
    if (!NimBLEDevice::getAdvertising()->isAdvertising()) {
        LOG("[WARN] BLE Advertising Stopped! Restarting...");
        NimBLEDevice::getAdvertising()->start(0);  // Restart indefinitely
    }

    /*
    // ✅ Improved WiFi reconnection logic
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastReconnectAttempt > 5000) {  // Retry every 5 seconds
            lastReconnectAttempt = millis();
            LOGF("⚠️ WiFi disconnected! Attempting reconnect at %lu ms", millis());
            WiFi.reconnect();
        }
    }
*/
    delay(100);  // Reduce CPU usage instead of `sleep(0.1)`
}

// ✅ Timer Callback Function (Runs Every 2 Seconds When Connected)
void sendFTMSUpdate(void* arg) {
    if (!isBLEConnected) {
        LOG("[DEBUG] No BLE connection, skipping FTMS update.");
        return;
    }

    FTMSDataStorage ftmsData = antParser.getFTMSData();

    bleFTMS.sendIndoorBikeData(ftmsData);
    LOGF("[DEBUG] BLE FTMS Update: Power=%dW, Speed=%d km/h, Cadence=%d rpm, Distance=%d m, Resistance=%.1f, Elapsed Time=%d s",
        ftmsData.power, ftmsData.speed, ftmsData.cadence, ftmsData.distance, ftmsData.resistance, ftmsData.elapsed_time);
}

// ✅ BLE Connect Callback → Start Sending Data
void onBLEConnect() {
    LOG("[INFO] BLE Device Connected! Starting FTMS updates.");
    isBLEConnected = true;
    esp_timer_start_periodic(ftmsTimer, FTMS_UPDATE_INTERVAL_US);  // ✅ Start Timer
}

// ✅ BLE Disconnect Callback → Stop Sending Data
void onBLEDisconnect() {
    LOG("[INFO] BLE Device Disconnected! Stopping FTMS updates.");
    isBLEConnected = false;
    esp_timer_stop(ftmsTimer);  // ✅ Stop Timer
    antParser.resetFTMData();  // Reset FTMS data
}

// ✅ Function to Listen for "Reboot" Command
void checkForReboot() {
    static char inputBuffer[10];  // Small buffer for command
    static uint8_t index = 0;

    while (logger.available()) {
        char c = logger.read();
        logger.print("[DEBUG] Received char: ");
        logger.println(c);

        // ✅ Store character unless buffer is full
        if (index < sizeof(inputBuffer) - 1) {
            inputBuffer[index++] = c;
            inputBuffer[index] = '\0';  // Null-terminate string
        }

        // ✅ Check if buffer contains "reboot"
        if (strcasecmp(inputBuffer, "reboot") == 0) {
            logger.println("[DEBUG] Reboot command received! Restarting ESP32...");
            delay(500);
            WiFi.disconnect(true, true);
            logger.flush();
            esp_restart();  // Perform software reboot
        }
    }
}


