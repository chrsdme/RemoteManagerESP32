#include <RemoteManager.h>

// Initialize with custom ports (default: OTA=8080, Debug=8081)
RemoteManager remoteManager;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  remoteManager.begin();
  remoteManager.handleOTA();
  remoteManager.handleDebugging();
  remoteManager.handleFileSystem();
}

void loop() {
  // Your application logic here
}
