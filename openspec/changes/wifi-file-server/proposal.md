## Why

Currently, managing files on the ebook reader's SD card requires physically removing the card and using a computer with a card reader. This is inconvenient for uploading new books, removing unwanted files, or reorganizing the library. By enabling the ESP32 to act as a WiFi Access Point with a built-in file server and web UI, users can manage SD card contents wirelessly from any device with a browser — no card removal needed.

## What Changes

- **New: WiFi Access Point mode** — ESP32 starts a WiFi AP with a configurable SSID and password when the `webserver` setting is enabled.
- **New: HTTP file server** — An async web server serves a web UI and exposes REST-like endpoints for SD card file operations (list, navigate, upload, delete).
- **New: Web UI** — A single-page HTML/CSS/JS interface (served from flash or embedded in code) that lets the user browse the SD card root, navigate into directories, upload files, and delete files/folders.
- **Integration with SettingsManager** — The existing `webserver` boolean setting gates whether the AP and server start. When toggled off, the AP and server are shut down.

## Capabilities

### New Capabilities

- `wifi-ap`: Manages ESP32 WiFi Access Point lifecycle — starting/stopping the AP, SSID/password configuration, and connection state.
- `file-server`: HTTP server exposing REST endpoints for SD card operations (directory listing, file download, file upload, file/folder deletion) and serving the web UI.
- `file-manager-webui`: Browser-based single-page interface for navigating the SD card filesystem, uploading files, and deleting files/folders.

### Modified Capabilities

- `dark-mode`: No requirement changes — dark mode only affects the e-ink display, not the web UI.

_No existing capabilities require spec-level requirement changes._

## Impact

- **New files**: `WebServerManager.h` / `WebServerManager.cpp` — new manager class following the existing pattern (like `SettingsManager`, `SDHandler`).
- **Modified files**:
  - `ebook_reader.ino` — initialize and manage the web server lifecycle in `setup()` and `loop()`.
- **Dependencies**: Requires `ESPAsyncWebServer` and `AsyncTCP` libraries (standard ESP32 async web server stack). These are well-maintained, widely used libraries for ESP32 Arduino projects.
- **Existing code**: `SDHandler` already provides all needed file operations (`listFiles`, `loadFile`, `saveFile`, `fileExists`, `createFolderRecursive`). The file server endpoints will delegate to `SDHandler` for SD card access.
- **Memory/resources**: The web server and WiFi AP consume additional RAM and CPU. The server should only run when explicitly enabled via the `webserver` setting to conserve resources during normal reading.
- **SD card concurrency**: When the web server is active, both the ebook reader UI and the web UI may access the SD card. Care is needed to avoid concurrent access issues (e.g., disable reader file operations while a web upload is in progress, or use simple mutex guarding).
