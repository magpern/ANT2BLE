#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

// ✅ Define logger instance
#ifdef DEBUG
    #define logger Serial1
    #define LOG(x) logger.println(x)
    #define LOGF(x, ...) logger.printf(x, ##__VA_ARGS__), logger.println()
#else
    #define logger if (false) Serial
    #define LOG(x)  // No-op in release
    #define LOGF(x, ...)  // No-op in release
#endif

// ✅ Declare function instead of defining it
extern int logger_vprintf(const char *format, va_list args);
extern void setupLogger();

#endif  // LOGGER_H
