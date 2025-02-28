#include <Arduino.h>
#include "ant_parser.h"
#include "ble_ftms.h"
#include "logger.h"
#include "esp_system.h"
#include "esp_timer.h"  // ✅ ESP32 Software Timer API

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

    // ✅ Ensure NimBLE logs go through logger
    setupLogger();
    esp_log_set_vprintf(logger_vprintf);  // Redirect all `printf` calls to use logger

    // Initialize BLE FTMS
    bleFTMS.begin();

    // Register BLE Callbacks
    bleFTMS.setConnectCallback(onBLEConnect);
    bleFTMS.setDisconnectCallback(onBLEDisconnect);

    // Print unique ESP32-S3 MAC address
    LOG("Device MAC: " + bleFTMS.getDeviceMAC());

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
    antParser.readSerial();
    checkForReboot();  // Check if "reboot" command is received

    // ✅ Restart advertising if it stops
    if (!NimBLEDevice::getAdvertising()->isAdvertising()) {
        LOG("[WARN] BLE Advertising Stopped! Restarting...");
        NimBLEDevice::getAdvertising()->start(0);  // Restart indefinitely
    }
}

// Mapper function to convert FTMSDataStorage to FTMSData
FTMSData mapFTMSData(const FTMSDataStorage& storage) {
    FTMSData data;
    data.elapsedTime = storage.elapsed_time;
    data.distance = storage.distance;
    data.speed = storage.speed;
    data.heartRate = storage.heart_rate;
    data.power = storage.power;
    data.virtualSpeed = storage.virtual_speed;
    data.accumulatedPower = storage.accumulated_power;
    data.instantaneousPower = storage.instantaneous_power;
    data.cadence = storage.cadence;
    data.cycleLength = storage.cycle_length;
    data.incline = storage.incline;
    data.resistance = storage.resistance;
    data.feState = storage.fe_state;
    data.manufacturerID = storage.manufacturerID;
    data.serialNumber = storage.serialNumber;
    data.softwareVersion = storage.softwareVersion;
    data.modelNumber = storage.modelNumber;
    data.hardwareRevision = storage.hardware_revision;
    data.trainerStatus = storage.trainer_status;
    data.maxResistance = storage.maxResistance;
    return data;
}

// ✅ Timer Callback Function (Runs Every 2 Seconds When Connected)
void sendFTMSUpdate(void* arg) {
    if (!isBLEConnected) {
        LOG("[DEBUG] No BLE connection, skipping FTMS update.");
        return;
    }

    FTMSData ftmsData = mapFTMSData(antParser.getFTMSData());

    bleFTMS.sendIndoorBikeData(ftmsData);
    LOGF("[DEBUG] BLE FTMS Update: Power=%dW, Speed=%d km/h, Cadence=%d rpm, Distance=%d m, Resistance=%d, Elapsed Time=%d s",
        ftmsData.power, ftmsData.speed, ftmsData.cadence, ftmsData.distance, ftmsData.resistance, ftmsData.elapsedTime);
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
}

// ✅ Function to Listen for "Reboot" Command
void checkForReboot() {
    static char inputBuffer[10];  // Small buffer for command
    static uint8_t index = 0;

    while (logger.available()) {
        char c = logger.read();
        logger.print("[DEBUG] Received: ");
        logger.println(c);  // Debug: Show each received character

        if (c == '\n' || c == '\r') {  // End of command
            inputBuffer[index] = '\0';  // Null-terminate string
            logger.print("[DEBUG] Full command: ");
            logger.println(inputBuffer);

            if (strcasecmp(inputBuffer, "reboot") == 0) {  // Case-insensitive compare
                logger.println("[DEBUG] Reboot command received! Restarting ESP32...");
                delay(500);
                logger.flush();  // Ensure log output is sent
                esp_restart();   // Perform software reboot
            }
            index = 0;  // Reset buffer index
        } else if (index < sizeof(inputBuffer) - 1) {
            inputBuffer[index++] = c;  // Store character
        }
    }

    // 🚀 EXTRA FIX: If full "reboot" command is detected *without newline*
    if (strcasecmp(inputBuffer, "reboot") == 0) {
        logger.println("[DEBUG] No newline, but command detected! Restarting...");
        delay(500);
        logger.flush();
        esp_restart();
    }
}
