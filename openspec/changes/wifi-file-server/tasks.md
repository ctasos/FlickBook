## 1. SDHandler Enhancements

- [x] 1.1 Add `listFilesWithMeta(path)` method to SDHandler that returns a vector of structs with `name`, `isDir`, and `size` fields for each directory entry
- [x] 1.2 Add `deletePath(path)` method to SDHandler that deletes a file, or recursively deletes a directory and its contents
- [x] 1.3 Add `renamePath(oldPath, newPath)` method to SDHandler that renames a file or directory using SdFat's `rename()`
- [x] 1.4 Add `openFileForRead(path)` and `getFileSize(path)` methods to SDHandler for streaming file access (returns SdFile handle or reads chunks into a buffer)

## 2. WebServerManager Core

- [x] 2.1 Create `WebServerManager.h` with class declaration: constructor(`Inkplate*`), `start()`, `stop()`, `handleClient()`, `isRunning()`, private `WebServer` instance and `bool running` state
- [x] 2.2 Create `WebServerManager.cpp` with `start()` implementation: call `WiFi.softAP("EbookReader", "ebookreader")`, create WebServer on port 80, register route handlers, call `server.begin()`, set `running = true`
- [x] 2.3 Implement `stop()`: call `server.stop()`, `WiFi.softAPdisconnect(true)`, set `running = false`
- [x] 2.4 Implement `handleClient()`: guard with `if (!running) return;`, then delegate to `server.handleClient()`
- [x] 2.5 Add path validation helper: reject paths containing `..`, normalize double slashes and trailing slashes

## 3. API Endpoints

- [x] 3.1 Implement `GET /` handler: serve the PROGMEM HTML page with content type `text/html`
- [x] 3.2 Implement `GET /api/list` handler: read `path` query param (default `/`), call `SDHandler::listFilesWithMeta()`, return JSON array with `name`, `isDir`, `size`
- [x] 3.3 Implement `GET /api/download` handler: validate path, open file via SDHandler, stream in chunks with `application/octet-stream` content type and `Content-Disposition` header
- [x] 3.4 Implement `POST /api/upload` handler: use WebServer's file upload callback to write chunks to SD card via SDHandler, create target directory if needed
- [x] 3.5 Implement `DELETE /api/delete` handler: validate path, call `SDHandler::deletePath()`, return JSON response
- [x] 3.6 Implement `POST /api/mkdir` handler: validate path, call `SDHandler::createFolderRecursive()`, return JSON response
- [x] 3.7 Implement `POST /api/rename` handler: validate path and newname params, call `SDHandler::renamePath()`, handle 404/409 error cases
- [x] 3.8 Add path traversal validation to all API handlers (reject `..` segments, normalize paths)

## 4. Web UI

- [x] 4.1 Create `WebUI.h` with a `PROGMEM` string constant containing the complete HTML page structure (head, body, container divs)
- [x] 4.2 Implement inline CSS: responsive layout, file listing table/list styles, action buttons, breadcrumb, upload area, mobile-friendly viewport
- [x] 4.3 Implement JS: `fetchListing(path)` function that calls `/api/list` and renders the directory entries with name, type icon/label, and formatted file size
- [x] 4.4 Implement JS: directory navigation (click folder to enter, breadcrumb/back button to go up), track current path state
- [x] 4.5 Implement JS: file upload via form with multipart POST to `/api/upload`, show progress indicator, refresh listing on success
- [x] 4.6 Implement JS: delete action with confirmation prompt, send DELETE to `/api/delete`, refresh listing on success
- [x] 4.7 Implement JS: download action triggering browser download from `/api/download`
- [x] 4.8 Implement JS: create folder action with name input prompt, POST to `/api/mkdir`, refresh listing
- [x] 4.9 Implement JS: rename action with name input prompt, POST to `/api/rename`, handle 409 conflict, refresh listing
- [x] 4.10 Implement JS: error handling for all API responses (display error messages, handle network failures)

## 5. Integration

- [x] 5.1 Add `#include "WebServerManager.h"` and global `WebServerManager webServerManager(&display)` in `ebook_reader.ino`
- [x] 5.2 In `setup()`: call `webServerManager.start()` if `settingsManager.getWebserver()` is `true`
- [x] 5.3 In `loop()`: add `webServerManager.handleClient()` call alongside `uiManager.handleTouch()`
- [x] 5.4 In `UIManagerTouch.cpp` webserver toggle handler: call `webServerManager.start()` or `webServerManager.stop()` when the setting is toggled
- [x] 5.5 In `UIManagerRendering.cpp` settings page: display SSID, password, and IP (`192.168.4.1`) below the webserver toggle when the server is active
