#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include "Arduino.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"

extern AsyncWebSocket ws;

void startWebSocketServer();
void onWebSocketMessage(void *arg, uint8_t *data, size_t len);

#endif // WEBSOCKET_MANAGER_H
