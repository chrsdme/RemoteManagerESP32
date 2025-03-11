# RemoteManager for ESP32  
**A modular library for OTA updates, remote debugging, and file system management on ESP32 devices**  

    
---

## Overview  
RemoteManager is a lightweight, modular library for ESP32-based projects that provides:  
- **Secure Over-the-Air (OTA) updates** with support for firmware (`fw.bin`), SPIFFS (`fs.bin`), and full (`all.bin`) updates.  
- **Remote debugging** via a web-based serial console (Server-Sent Events).  
- **File system management** (upload, download, delete, list files in SPIFFS).  
- **Zero dependencies** beyond core ESP32 frameworks (WiFi, SPIFFS, AsyncTCP).  

---

## Features  
✅ **Differentiated OTA Updates**  
- Automatically detects `fw.bin` (firmware), `fs.bin` (SPIFFS), or `all.bin` (full update) for seamless partition management.  
- Secure updates with checksum validation and automatic rollback on failure.  

✅ **Real-Time Debugging**  
- Stream serial output to a web browser via `/stream` (SSE).  
- Poll-based debugging via `/debug` endpoint.  

✅ **File System Operations**  
- List files: `GET /list`  
- Upload files: `POST /upload`  
- Download files: `GET /download?file=<filename>`  
- Delete files: `GET /delete?file=<filename>`  

✅ **Security**  
- Credentials stored in `secrets.h` (never hardcoded).  
- HTTPS-ready (certificates configurable via `secrets.h`).  

---

## Installation  
1. **PlatformIO**:  
   ```ini
   [env:esp32dev]
   platform = espressif32
   framework = arduino
   lib_deps = 
       [https://github.com/yourusername/RemoteManager.git]
