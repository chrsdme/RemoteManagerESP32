#include "RemoteManager.h"

// Constructor
RemoteManager::RemoteManager(uint16_t otaPort, uint16_t debugPort)
    : _otaPort(otaPort), _debugPort(debugPort), _otaServer(_otaPort), _debugServer(_debugPort) {}

// Initialize the manager
void RemoteManager::begin() {
    // Start SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    // Setup servers
    _setupOTAServer();
    _setupDebugServer();
    _setupFileSystemServer();

    Serial.println("RemoteManager initialized.");
}

// Handle OTA updates
void RemoteManager::handleOTA() {
    _otaServer.begin();
}

// Handle remote debugging
void RemoteManager::handleDebugging() {
    _debugServer.on("/debug", HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", _debugBuffer.c_str());
        _debugBuffer = ""; // Clear buffer after sending
    });

    _debugServer.on("/stream", HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->send(200, "text/event-stream", "", [&](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            if (_debugBuffer.length() > 0) {
                size_t len = _debugBuffer.length();
                memcpy(buffer, _debugBuffer.c_str(), len);
                _debugBuffer = ""; // Clear buffer after sending
                return len;
            }
            return 0;
        });
    });

    _debugServer.begin();
}

// Handle file system operations
void RemoteManager::handleFileSystem() {
    // List files
    _otaServer.on("/list", HTTP_GET, [&](AsyncWebServerRequest *request) {
        String fileList = "";
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while (file) {
            fileList += file.name();
            fileList += "\n";
            file = root.openNextFile();
        }
        request->send(200, "text/plain", fileList);
    });

    // Upload file
    _otaServer.on("/upload", HTTP_POST, [&](AsyncWebServerRequest *request) {}, NULL,
                  [&](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
                      if (!index) {
                          request->_tempFile = SPIFFS.open("/" + filename, "w");
                      }
                      if (len) {
                          request->_tempFile.write(data, len);
                      }
                      if (final) {
                          request->_tempFile.close();
                          request->send(200, "text/plain", "File uploaded: " + filename);
                      }
                  });

    // Download file
    _otaServer.on("/download", HTTP_GET, [&](AsyncWebServerRequest *request) {
        if (request->hasParam("file")) {
            String filename = request->getParam("file")->value();
            if (SPIFFS.exists(filename)) {
                request->send(SPIFFS, filename, "application/octet-stream");
            } else {
                request->send(404, "text/plain", "File not found");
            }
        } else {
            request->send(400, "text/plain", "Missing file parameter");
        }
    });

    // Delete file
    _otaServer.on("/delete", HTTP_GET, [&](AsyncWebServerRequest *request) {
        if (request->hasParam("file")) {
            String filename = request->getParam("file")->value();
            if (SPIFFS.exists(filename)) {
                SPIFFS.remove(filename);
                request->send(200, "text/plain", "File deleted: " + filename);
            } else {
                request->send(404, "text/plain", "File not found");
            }
        } else {
            request->send(400, "text/plain", "Missing file parameter");
        }
    });

    _otaServer.begin();
}

// Setup OTA server with differentiated update handling
void RemoteManager::_setupOTAServer() {
    _otaServer.on("/update", HTTP_POST, [&](AsyncWebServerRequest *request) {},
                  [&](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
                      static bool isFirmwareUpdate = false;
                      static bool isSPIFFSUpdate = false;

                      // Determine the type of update based on the filename
                      if (!index) {
                          if (filename.endsWith(".bin")) {
                              if (filename == "fw.bin") {
                                  Serial.println("Starting Firmware Update...");
                                  Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
                                  isFirmwareUpdate = true;
                              } else if (filename == "fs.bin") {
                                  Serial.println("Starting SPIFFS Update...");
                                  Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS);
                                  isSPIFFSUpdate = true;
                              } else if (filename == "all.bin") {
                                  Serial.println("Starting Full Update...");
                                  Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
                                  isFirmwareUpdate = true; // First write firmware
                                  isSPIFFSUpdate = true;   // Then write SPIFFS
                              } else {
                                  request->send(400, "text/plain", "Invalid file type");
                                  return;
                              }
                          } else {
                              request->send(400, "text/plain", "Invalid file type");
                              return;
                          }
                      }

                      // Write data chunk
                      if (len) {
                          if (!Update.write(data, len)) {
                              request->send(500, "text/plain", "Update write failed");
                              return;
                          }
                      }

                      // Finalize update
                      if (final) {
                          if (Update.end(true)) {
                              if (isFirmwareUpdate && !isSPIFFSUpdate) {
                                  request->send(200, "text/plain", "Firmware Update successful. Rebooting...");
                                  delay(1000);
                                  ESP.restart();
                              } else if (isSPIFFSUpdate && !isFirmwareUpdate) {
                                  request->send(200, "text/plain", "SPIFFS Update successful. Rebooting...");
                                  delay(1000);
                                  ESP.restart();
                              } else if (isFirmwareUpdate && isSPIFFSUpdate) {
                                  // Switch to SPIFFS partition for the second part of the update
                                  Serial.println("Switching to SPIFFS Update...");
                                  Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS);
                                  isFirmwareUpdate = false; // Reset flag
                              } else {
                                  request->send(200, "text/plain", "Full Update successful. Rebooting...");
                                  delay(1000);
                                  ESP.restart();
                              }
                          } else {
                              request->send(500, "text/plain", "Update failed");
                          }
                      }
                  });
}

// Setup debug server
void RemoteManager::_setupDebugServer() {
    _debugServer.on("/stream", HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->send(200, "text/event-stream", "", [&](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            if (_debugBuffer.length() > 0) {
                size_t len = _debugBuffer.length();
                memcpy(buffer, _debugBuffer.c_str(), len);
                _debugBuffer = ""; // Clear buffer after sending
                return len;
            }
            return 0;
        });
    });

    // Redirect serial output to debug buffer
    Serial.setDebugOutput(true);
    Serial.onLog([](int level, const char *tag, const char *format, va_list args) {
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        _debugBuffer += buffer;
    });
}

// Setup file system server
void RemoteManager::_setupFileSystemServer() {
    // Implemented in handleFileSystem()
}
