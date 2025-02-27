#include "logger.h"
#include "esp_log.h"

// ✅ Define `logger_vprintf` only in `logger.cpp`
int logger_vprintf(const char *format, va_list args) {
    char buffer[256];  // Buffer to hold formatted text
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    if (len > 0) {
        logger.print(buffer);  // Print formatted text
        if (buffer[len - 1] != '\n') {  // ✅ Ensure newline is added
            logger.println();
        }
    }
    return len;
}

// ✅ Setup NimBLE & ESP-IDF Logging
void setupLogger() {
    esp_log_set_vprintf(logger_vprintf);
    esp_log_level_set("*", ESP_LOG_DEBUG);  // ✅ Set global log level to DEBUG
}
