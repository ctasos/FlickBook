## Context

The ebook reader runs on an ESP32-based Inkplate 6 board. The codebase follows a manager pattern — global singleton-style classes (`SettingsManager`, `SDHandler`, `LibraryManager`, `UIManager`) initialized in `setup()` and coordinated through `loop()`. SD card access uses the SdFat library through `SDHandler`. The `webserver` boolean setting already exists in `SettingsManager` with a toggle in the settings UI, but no server implementation exists yet.

The ESP32 has built-in WiFi hardware and ~300KB of usable RAM. The Inkplate library already depends on WiFi.h internally. The main loop is single-threaded, driven by `uiManager.handleTouch()`.

## Goals / Non-Goals

**Goals:**

- ESP32 runs a WiFi AP + HTTP file server when the `webserver` setting is toggled on
- Users can browse, upload, rename and delete files on the SD card from a browser
- The server starts/stops cleanly when the setting is toggled, freeing resources when off
- Follows the existing manager pattern (`WebServerManager` as a new global manager)
- Web UI is self-contained (embedded in flash, no external dependencies)

**Non-Goals:**

- Station mode (connecting to existing WiFi networks) — AP-only for simplicity
- Authentication/HTTPS — this is a local-only AP with no internet access
- Editing file contents in the browser — only filesystem operations (browse, upload, delete)
- Streaming or previewing ebook content in the browser
- File move operations (out of scope for first version)
- OTA firmware updates through the web UI

## Decisions

### 1. New `WebServerManager` class following existing manager pattern

**Choice:** Create `WebServerManager.h` / `WebServerManager.cpp` as a new manager, instantiated as a global in `ebook_reader.ino` like other managers.

**Rationale:** Consistent with `SettingsManager`, `SDHandler`, `LibraryManager`. Keeps WiFi + server logic isolated from the rest of the codebase. The class owns the AP and server lifecycle.

**Alternative considered:** Adding server code directly to `SettingsManager` or `UIManager`. Rejected — violates single responsibility and would bloat existing classes.

### 2. Use ESP32 built-in WiFi AP (no external WiFi library)

**Choice:** Use `WiFi.softAP()` / `WiFi.softAPdisconnect()` from the ESP32 WiFi library (already available via the Inkplate library dependency chain).

**Rationale:** ESP32's built-in WiFi AP supports up to 4 simultaneous connections, sufficient for a single user managing files. No additional library needed — WiFi.h is already transitively included.

**Configuration:** Hardcoded SSID `"EbookReader"` and password `"ebookreader"`. Displayed on the e-ink screen when active.

### 3. Use the ESP32 built-in WebServer library (not AsyncWebServer)

**Choice:** Use `WebServer.h` (synchronous, included in ESP32 Arduino core) instead of ESPAsyncWebServer.

**Rationale:** The synchronous `WebServer` is already included in the ESP32 Arduino core — no additional library dependency. AsyncWebServer would add ~60KB of flash and two new library dependencies (`ESPAsyncWebServer` + `AsyncTCP`). Since the AP handles at most 1-2 concurrent users and the server runs in `loop()`, synchronous handling is sufficient. This keeps the dependency footprint minimal, matching the project's lean approach.

**Alternative considered:** ESPAsyncWebServer — better for high concurrency but adds external dependencies and complexity. Overkill for a single-user file manager on a local AP.

### 4. Web UI embedded as a raw string literal in program memory (PROGMEM)

**Choice:** Embed the HTML/CSS/JS as a `PROGMEM` string constant in a header file (`WebUI.h`), served from the `/` endpoint.

**Rationale:** The web UI is a single self-contained HTML page (~5-10KB) with inline CSS and JS. Embedding it in PROGMEM avoids SD card reads for serving the UI and works even if the SD card is being written to. This is the standard pattern for ESP32 web server projects.

**Alternative considered:** Serving HTML files from SD card. Rejected — creates a dependency on specific SD card contents and complicates deployment (users would need to manage UI files on the SD card).

### 5. REST-like API endpoints using SDHandler

**Choice:** The web server exposes these endpoints, all delegating to the existing `SDHandler` methods:

| Endpoint | Method | SDHandler method | Description |
|---|---|---|---|
| `/` | GET | — | Serve the web UI page |
| `/api/list?path=<dir>` | GET | `listFiles()` | List directory contents (returns JSON) |
| `/api/download?path=<file>` | GET | — | Download a file (stream from SD via SdFat) |
| `/api/upload?path=<dir>` | POST | `saveFile()` | Upload a file to a directory |
| `/api/delete?path=<file>` | DELETE | — | Delete a file or folder |
| `/api/mkdir?path=<dir>` | POST | `createFolderRecursive()` | Create a new directory |
| `/api/rename?path=<file>&newname=<new_name>` | POST | Renames a file |

**Rationale:** JSON API with a single-page UI keeps the interface clean and decoupled. Using query parameters for paths is simple and works well with the browser's fetch API.

**Note on SDHandler:** `SDHandler::listFiles()` returns a flat `std::vector<String>` of names without distinguishing files from directories. The `/api/list` endpoint will need to open each entry and check `isDir()` to provide this information in the JSON response. This may require a new helper methods in SDHandler for the directory listing endpoint specifically. Enhance the SDHandler with the methods needed to interact with the WebUI.

### 6. SD card access: direct SdFat for streaming, SDHandler for metadata

**Choice:** For file upload/download (which involve streaming), `WebServerManager` add required functions in the SDHandler to  access the SdFat filesystem directly (via `display->getSdFat()` or similar based on current codebase) to avoid loading entire files into RAM. For metadata operations (exists, mkdir, list), also delegate to `SDHandler` or enhance it when needed.

**Rationale:** `SDHandler::loadFile()` reads the entire file into a `String` — unsuitable for large files (EPUBs can be 1-5MB, RAM is ~300KB). Streaming reads/writes directly from SdFat avoids RAM exhaustion. File uploads must also be streamed chunk-by-chunk.

### 7. Server lifecycle tied to settings toggle

**Choice:** When the `webserver` setting is toggled ON in the settings UI, `WebServerManager::start()` is called. When toggled OFF, `WebServerManager::stop()` is called. The `loop()` function calls `WebServerManager::handleClient()` only when the server is active.

**Rationale:** Clean start/stop lifecycle. WiFi AP and HTTP server are only running when needed, conserving power and RAM during normal reading. The toggle already exists in the UI — just needs to trigger the server lifecycle.

### 9. No SD card mutex — sequential access model

**Choice:** Do not add a mutex or semaphore. The synchronous `WebServer` processes one request at a time in `loop()`, and touch handling runs in the same loop iteration. SD card access is inherently serialized.

**Rationale:** With the synchronous WebServer, `handleClient()` and `handleTouch()` run sequentially in `loop()`. There's no true concurrency — only one operation accesses the SD card at any given time. A mutex would add complexity for no benefit.

**Risk:** If a large file upload takes a long time, the UI will be unresponsive during that period. This is acceptable for a file management utility that runs temporarily.

## Risks / Trade-offs

- **RAM pressure** → The ESP32 has ~300KB usable RAM. The WiFi stack + WebServer consume ~40-50KB when active. Mitigation: server only runs when explicitly enabled; streaming file transfers avoid loading files into RAM.
- **UI responsiveness during transfers** → Large file uploads block the main loop. Mitigation: acceptable trade-off since the server is a utility mode, not the primary reading experience. Users expect to pause reading while managing files.
- **Large file uploads** → ESP32 WebServer has default body size limits. Mitigation: configure max upload size; chunk uploads in the web UI if needed; provide progress feedback in the browser.
- **SdFat library thread safety** → SdFat is not thread-safe, but with the synchronous WebServer this is not an issue (single-threaded access). If the project later moves to AsyncWebServer, this would need revisiting.
- **Flash usage** → Embedded HTML adds ~5-10KB to flash. The ESP32 has 4MB flash — negligible impact.
