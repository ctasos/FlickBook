## ADDED Requirements

### Requirement: WiFi AP starts when webserver setting is enabled
The system SHALL start a WiFi Access Point when `settingsManager.getWebserver()` returns `true` and `WebServerManager::start()` is called. The AP SHALL use SSID `"EbookReader"` and password `"ebookreader"`.

#### Scenario: Start AP on settings toggle
- **WHEN** the user enables the `webserver` setting from the Settings screen
- **THEN** the system SHALL call `WebServerManager::start()`, which starts a WiFi AP with SSID `"EbookReader"` and password `"ebookreader"`
- **AND** the AP SHALL be reachable at IP address `192.168.4.1`

#### Scenario: AP not started on boot when setting is off
- **WHEN** the device boots and `settingsManager.getWebserver()` returns `false`
- **THEN** the WiFi AP SHALL NOT be started and `WiFi.softAP()` SHALL NOT be called

#### Scenario: AP started on boot when setting is on
- **WHEN** the device boots and `settingsManager.getWebserver()` returns `true`
- **THEN** the system SHALL automatically start the WiFi AP during initialization

### Requirement: WiFi AP stops when webserver setting is disabled
The system SHALL stop the WiFi Access Point when `settingsManager.getWebserver()` returns `false` and `WebServerManager::stop()` is called.

#### Scenario: Stop AP on settings toggle
- **WHEN** the user disables the `webserver` setting from the Settings screen
- **THEN** the system SHALL call `WebServerManager::stop()`, which disconnects the WiFi AP via `WiFi.softAPdisconnect(true)`
- **AND** the WiFi radio SHALL be turned off to conserve power

#### Scenario: Connected clients disconnected on stop
- **WHEN** the AP is stopped while a client is connected
- **THEN** all connected clients SHALL be disconnected

### Requirement: WebServerManager follows existing manager pattern
`WebServerManager` SHALL be implemented as a class with a constructor accepting an `Inkplate*` pointer, following the same pattern as `SettingsManager` and `SDHandler`. It SHALL be declared as a global instance in `ebook_reader.ino`.

#### Scenario: Global instantiation
- **WHEN** the firmware compiles
- **THEN** `WebServerManager` SHALL be instantiated as a global object in `ebook_reader.ino` alongside existing managers

#### Scenario: Start and stop lifecycle
- **WHEN** `start()` is called
- **THEN** the WiFi AP SHALL be started and the HTTP server SHALL begin listening on port 80
- **WHEN** `stop()` is called
- **THEN** the HTTP server SHALL stop and the WiFi AP SHALL be disconnected

### Requirement: Server active state is queryable
`WebServerManager` SHALL expose an `isRunning()` method that returns `true` when both the AP and HTTP server are active.

#### Scenario: Query running state when active
- **WHEN** `start()` has been called successfully
- **THEN** `isRunning()` SHALL return `true`

#### Scenario: Query running state when stopped
- **WHEN** `stop()` has been called or `start()` was never called
- **THEN** `isRunning()` SHALL return `false`

### Requirement: Handle client requests in main loop
The `loop()` function in `ebook_reader.ino` SHALL call `WebServerManager::handleClient()` on each iteration when the server is running. This method SHALL delegate to the underlying `WebServer::handleClient()`.

#### Scenario: Loop processes HTTP requests
- **WHEN** the server is running and a client sends an HTTP request
- **THEN** `handleClient()` called from `loop()` SHALL process the request and send a response

#### Scenario: Loop skips handling when server is off
- **WHEN** the server is not running
- **THEN** `handleClient()` SHALL return immediately without processing
