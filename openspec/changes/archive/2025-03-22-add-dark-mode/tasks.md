## 1. SettingsManager – dark mode setting & color helpers

- [x] 1.1 Add `bool darkMode` private field to `SettingsManager` in `SettingsManager.h`
- [x] 1.2 Add `getDarkMode()`, `setDarkMode(bool)`, `getFgColor()`, `getBgColor()` declarations to `SettingsManager.h`
- [x] 1.3 Initialise `darkMode` to `false` in the `SettingsManager` constructor in `SettingsManager.cpp`
- [x] 1.4 Implement `getDarkMode()` and `setDarkMode(bool)` following the `getGestures`/`setGestures` pattern in `SettingsManager.cpp`
- [x] 1.5 Implement `getFgColor()` / `getBgColor()` — return `BLACK`/`WHITE` when `darkMode` is false, swap when true
- [x] 1.6 Add `"darkMode"` key to `loadSettings()` and `saveSettings()` in `SettingsManager.cpp`

## 2. UIManagerRendering – replace hard-coded colors

- [x] 2.1 In `renderMenu()`, replace every `BLACK`/`WHITE` fill/draw color with `settingsManager.getFgColor()`/`settingsManager.getBgColor()`
- [x] 2.2 In `renderMenu()`, XOR every `drawIcon` `invert` argument with `settingsManager.getDarkMode()`
- [x] 2.3 In `renderMainHeader()`, replace `BLACK`/`WHITE` fill and text colors with fg/bg helpers and XOR icon `invert`
- [x] 2.4 In `renderReadingHeader()`, replace `BLACK`/`WHITE` fill and text colors with fg/bg helpers and XOR icon `invert`
- [x] 2.5 In `renderBookList()`, replace `BLACK` draw/text color with `settingsManager.getFgColor()` and XOR icon `invert` for finished-book checkbox icon
- [x] 2.6 In `renderSettings()`, replace `BLACK`/`WHITE` fill/draw/text colors with fg/bg helpers and XOR icon `invert` arguments; override `SETTINGS_SEP_*` color element with `settingsManager.getFgColor()` at the draw call
- [x] 2.7 In `renderLoadingMsg()`, replace `BLACK`/`WHITE` colors with fg/bg helpers
- [x] 2.8 In `renderLoadingIcon()`, XOR `drawIcon` `invert` with `settingsManager.getDarkMode()`
- [x] 2.9 In `drawBattery()`, replace `WHITE` text color with `settingsManager.getBgColor()` and XOR `drawIcon` `invert`

## 3. UIManagerTextLayout – replace hard-coded colors

- [x] 3.1 In `UIManagerTextLayout.cpp`, replace `BLACK` in any `setTextColor` call with `settingsManager.getFgColor()`

## 4. SnakeGame – pass runtime colors

- [x] 4.1 Add `int fgColor` and `int bgColor` fields to `SnakeGame` and accept them as `begin()` parameters in `SnakeGame.h`
- [x] 4.2 Store the color values in `begin()` and replace all `BLACK`/`WHITE` usages in `SnakeGame.cpp` with `fgColor`/`bgColor`
- [x] 4.3 In `UIManager.cpp`, pass `settingsManager.getFgColor()` and `settingsManager.getBgColor()` when calling `snakeGame->begin()`

## 5. Settings screen – Dark Mode toggle UI & touch

- [x] 5.1 Add `SETTINGS_ITEM_4[]` and `SETTINGS_SEP_3[]` constants in `UIManager.h` following the existing pattern
- [x] 5.2 In `renderSettings()` in `UIManagerRendering.cpp`, add the "Dark Mode" label, status icon, and separator using the new constants
- [x] 5.3 In `handleTouchSettingsPage()` in `UIManagerTouch.cpp`, add a touch handler for the Dark Mode toggle icon that calls `settingsManager.setDarkMode(!settingsManager.getDarkMode())` and re-renders with a full refresh

## 6. Full refresh on mode change

- [x] 6.1 Ensure the `renderScreen` call after toggling dark mode uses `partial_update = false` to force `display->display()` instead of `partialUpdate()`
