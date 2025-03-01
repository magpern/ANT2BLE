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
    class NullLogger {
    public:
        void begin(...) {}
        void print(...) {}
        void println(...) {}
        void printf(...) {}
        void flush() {}
        bool available() { return false; }  // Prevents errors in `while (logger.available())`
        int read() { return -1; }  // Simulate no input
    };

    extern NullLogger logger;  // Declare logger as a NullLogger instance
    #define LOG(x)  // No-op in release
    #define LOGF(x, ...)  // No-op in release
#endif

#endif  // LOGGER_H
