#include "websocket_manager.h"
#include "logger.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    LOGF("Received WebSocket Data: %s\n", data);
    // Parse and use ANT+ data here
}

void startWebSocketServer() {
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_DATA) {
            onWebSocketMessage(arg, data, len);
        }
    });

    server.addHandler(&ws);
    server.begin();
    Serial.println("WebSocket Server Started!");
}
