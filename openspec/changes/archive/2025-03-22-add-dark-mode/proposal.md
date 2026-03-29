## Why

The e-book reader currently only supports a light (Day Mode) appearance — white backgrounds with black text and UI elements. Users reading in low-light environments would benefit from a Dark Mode that inverts all colors, reducing eye strain. Adding it as a persistent setting managed by `SettingsManager` keeps it consistent with how other preferences (gestures, backlight, webserver) are already handled.

## What Changes

- Add a new `darkMode` boolean setting to `SettingsManager`, persisted in `settings.json`, with getter/setter following the existing pattern (`getGestures` / `setGestures`).
- Replace all hard-coded `BLACK` and `WHITE` color references in the rendering code (`UIManagerRendering.cpp`, `SnakeGame.cpp`, and the `SETTINGS_SEP_*` constants in `UIManager.h`) with two runtime-resolved color helpers (e.g. `FG_COLOR` / `BG_COLOR`) that return the correct value based on `darkMode`.
- Swap the `invert` flag on `drawIcon` / `renderImage` calls so icons render correctly in both modes.
- Expose the toggle in the Settings screen as a new settings item (same pattern as Gestures / Webserver toggles) so the user can switch modes from the UI and the choice is saved across reboots.

## Capabilities

### New Capabilities
- `dark-mode`: Covers the new dark-mode setting, runtime color resolution, icon inversion logic, and settings-screen toggle. One cohesive capability because all pieces depend on the same boolean flag.

### Modified Capabilities
_None — no existing specs to modify._

## Impact

- **Files changed**: `SettingsManager.h/cpp`, `UIManager.h`, `UIManagerRendering.cpp`, `UIManagerTextLayout.cpp` (if it uses color constants), `SnakeGame.cpp/h`, `sdcard/settings.json` (new key `darkMode`).
- **No new dependencies** — uses only the existing Inkplate 6 Flick display API (`setTextColor`, `fillRect`, `drawImage` invert parameter, etc.).
- **No breaking changes** — default value `false` (Day Mode) preserves current behaviour on first boot and for users with existing `settings.json` files that lack the key.
