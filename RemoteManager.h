#ifndef REMOTE_MANAGER_H
#define REMOTE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPUpdate.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Include secrets for configuration
#include "secrets.h"

class RemoteManager {
public:
    // Constructor
    RemoteManager(uint16_t otaPort = 8080, uint16_t debugPort = 8081);

    // Initialize the manager
    void begin();

    // Handle OTA updates
    void handleOTA();

    // Handle remote debugging (serial over WiFi)
    void handleDebugging();

    // Handle file system operations
    void handleFileSystem();

private:
    // Ports for services
    uint16_t _otaPort;
    uint16_t _debugPort;

    // Web server instances
    AsyncWebServer _otaServer;
    AsyncWebServer _debugServer;

    // Debugging buffer
    String _debugBuffer;

    // Helper functions
    void _setupOTAServer();
    void _setupDebugServer();
    void _setupFileSystemServer();
};

#endif
