## Context

The e-book reader runs on an Inkplate 6 Flick (ESP32 + e-ink). The display operates in either 1-bit (BW) or 3-bit grayscale mode, selected at compile time via the `GRAYSCALE` macro. In BW mode `BLACK = 1`, `WHITE = 0`; in grayscale mode `BLACK = 0`, `WHITE = 65535`. These are currently `#define` constants in `UIManager.h`.

All user-facing settings (gestures, backlight, webserver) follow an identical pattern: a private field in `SettingsManager`, a getter/setter pair, persistence in `/settings.json` via `SDHandler::saveJson`, and a toggle in the Settings screen with the `circle_check_icon` / `circle_blank_icon` status icons.

Icons and images are drawn with `display->drawImage(path, x, y, dither, invert)`. The `invert` flag flips the bitmap pixels at draw time, so a "dark icon on transparent" PNG renders as white-on-black when `invert = true`. This existing parameter is the key mechanism for Dark Mode icon rendering — no custom pixel manipulation is needed.

## Goals / Non-Goals

**Goals:**
- Provide a runtime-togglable Dark Mode that inverts the entire UI (backgrounds, text, borders, icons).
- Persist the preference across reboots in `settings.json`.
- Expose the toggle in the Settings screen alongside existing settings.
- Keep changes minimal and consistent with the current project style.

**Non-Goals:**
- Per-screen or per-element theming (all-or-nothing inversion).
- Changing the compile-time `GRAYSCALE` logic — Dark Mode works orthogonally to it.
- Adjusting fonts, sizes, or layout — only colors change.

## Decisions

### 1. Color access via `SettingsManager` helper methods instead of replacing `#define` constants

**Choice**: Add two inline helper methods to `SettingsManager` — `getFgColor()` and `getBgColor()` — that return the correct foreground/background value based on the `darkMode` flag. Keep `BLACK`/`WHITE` `#define`s unchanged (they still represent the hardware color values).

**Rationale**: Replacing the `#define` macros with runtime variables would require changing every file that includes `UIManager.h` and break the `SETTINGS_SEP_*` compile-time `const` arrays (which embed `BLACK` in their initializer lists). Instead, rendering code calls `settingsManager.getFgColor()` / `settingsManager.getBgColor()` where it currently uses `BLACK` / `WHITE`. The `SETTINGS_SEP_*` arrays keep using the raw `BLACK` constant and the `renderSettings` function passes the correct runtime color at draw time.

**Alternative considered**: A global `FG_COLOR` / `BG_COLOR` macro — rejected because it would require a global pointer to `settingsManager`, breaking the current encapsulation where `settingsManager` is an `extern` only in `.cpp` files.

### 2. Icon inversion via XOR with darkMode flag

**Choice**: Where icons currently pass a literal `true` or `false` as the `invert` argument, replace with `darkMode != originalInvert` (i.e., XOR). This means:
- An icon meant to be white-on-black in Day Mode (`invert = true`) stays `true` in Day Mode and flips to `false` in Dark Mode.
- An icon meant to be dark-on-white (`invert = false`) stays `false` in Day Mode and flips to `true` in Dark Mode.

**Rationale**: XOR gives correct behaviour for both modes with a single expression, no `if/else` branching needed at each call site.

### 3. Settings toggle using existing settings-item pattern

**Choice**: Add a fourth item row in `renderSettings` (following `SETTINGS_ITEM_3` → new `SETTINGS_ITEM_4`), a new separator (`SETTINGS_SEP_3`), and a touch handler mirroring the Gestures/Webserver pattern.

**Rationale**: Exact reuse of the existing layout and interaction code. Minimal lines added.

### 4. Default value `false` (Day Mode)

**Choice**: `darkMode` defaults to `false` in the constructor. `loadSettings` reads `"darkMode"` from JSON; if the key is missing (existing installs), it stays `false`.

**Rationale**: Preserves current appearance for all existing users. No migration step needed — the key is simply added on next `saveSettings()` call.

## Risks / Trade-offs

- **SnakeGame** uses `BLACK` / `WHITE` directly. These need updating to `settingsManager.getFgColor()` / `settingsManager.getBgColor()`, which requires `SnakeGame` to receive or access the settings manager. → **Mitigation**: Pass foreground/background colors into `SnakeGame::begin()`.
- **Compile-time `const` arrays** (`SETTINGS_SEP_1`, `SETTINGS_SEP_2`) embed `BLACK` at compile time, so they can't adapt at runtime. → **Mitigation**: The drawing code already unpacks these arrays element by element, so override the color element with `settingsManager.getFgColor()` at the call site.
- **Partial updates on e-ink** may show ghosting when switching modes. → **Mitigation**: Force a full display refresh (`display->display()`) when the mode changes, same as a full screen render.

## Open Questions

_None — the approach is straightforward given the existing patterns._
