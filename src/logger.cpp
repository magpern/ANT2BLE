#include "logger.h"

// ✅ Ensure logger output always ends with a newline
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

