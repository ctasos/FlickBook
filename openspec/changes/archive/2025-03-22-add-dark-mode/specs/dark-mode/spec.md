## ADDED Requirements

### Requirement: Dark Mode setting in SettingsManager
The system SHALL expose a boolean `darkMode` setting in `SettingsManager` with a getter (`getDarkMode`) and a setter (`setDarkMode`), following the same pattern as `getGestures` / `setGestures`. The default value SHALL be `false` (Day Mode).

#### Scenario: Default value on first boot
- **WHEN** the device boots with a `settings.json` that does not contain a `"darkMode"` key
- **THEN** `getDarkMode()` SHALL return `false`

#### Scenario: Toggle dark mode on
- **WHEN** `setDarkMode(true)` is called
- **THEN** `getDarkMode()` SHALL return `true` and the value SHALL be persisted to `settings.json`

#### Scenario: Persist across reboot
- **WHEN** the user enables Dark Mode and the device reboots
- **THEN** `getDarkMode()` SHALL return `true` after `loadSettings()` completes

### Requirement: Foreground and background color helpers
`SettingsManager` SHALL provide `getFgColor()` and `getBgColor()` methods that return the correct display color values based on the current `darkMode` state.

#### Scenario: Day Mode colors
- **WHEN** `getDarkMode()` returns `false`
- **THEN** `getFgColor()` SHALL return `BLACK` and `getBgColor()` SHALL return `WHITE`

#### Scenario: Dark Mode colors
- **WHEN** `getDarkMode()` returns `true`
- **THEN** `getFgColor()` SHALL return `WHITE` and `getBgColor()` SHALL return `BLACK`

### Requirement: UI rendering uses runtime colors
All UI rendering code (headers, menus, book list, settings page, loading messages) SHALL use `settingsManager.getFgColor()` and `settingsManager.getBgColor()` instead of hard-coded `BLACK` and `WHITE` constants for foreground and background colors respectively.

#### Scenario: Main screen in Day Mode
- **WHEN** Dark Mode is off
- **THEN** the main screen SHALL render with black text/borders on a white background (current behaviour)

#### Scenario: Main screen in Dark Mode
- **WHEN** Dark Mode is on
- **THEN** the main screen SHALL render with white text/borders on a black background

#### Scenario: Reading screen in Dark Mode
- **WHEN** Dark Mode is on and the user is on the reading screen
- **THEN** page text SHALL render in white on a black background, and the header SHALL render with inverted colors compared to Day Mode

### Requirement: Icon inversion adapts to dark mode
All `drawIcon` and `renderImage` calls SHALL XOR the original `invert` flag with the `darkMode` state so icons display correctly in both modes.

#### Scenario: Icon on filled menu button in Day Mode
- **WHEN** Dark Mode is off and an icon is drawn on a black-filled menu button
- **THEN** the icon SHALL be drawn with `invert = true` (white icon on black)

#### Scenario: Icon on filled menu button in Dark Mode
- **WHEN** Dark Mode is on and an icon is drawn on a white-filled menu button
- **THEN** the icon SHALL be drawn with `invert = false` (black icon on white), because `darkMode != originalInvert` flips the value

#### Scenario: Icon on white background in Day Mode
- **WHEN** Dark Mode is off and an icon is drawn on a white background with original `invert = false`
- **THEN** the icon SHALL be drawn with `invert = false` (black icon)

#### Scenario: Icon on black background in Dark Mode
- **WHEN** Dark Mode is on and an icon is drawn on a black background with original `invert = false`
- **THEN** the icon SHALL be drawn with `invert = true` (white icon)

### Requirement: Settings screen toggle for Dark Mode
The Settings screen SHALL display a "Dark Mode" item following the same layout pattern as Gestures and Webserver items, with a `circle_check_icon` / `circle_blank_icon` status indicator.

#### Scenario: Toggle visible on settings screen
- **WHEN** the user navigates to the Settings screen
- **THEN** a "Dark Mode" row SHALL be visible with the current state shown via the check/blank circle icon

#### Scenario: Toggle dark mode from settings
- **WHEN** the user taps the Dark Mode toggle icon on the Settings screen
- **THEN** `darkMode` SHALL be toggled, the setting SHALL be saved, and the screen SHALL re-render with the new color scheme using a full display refresh

### Requirement: SnakeGame respects dark mode colors
`SnakeGame` SHALL use the current foreground and background colors from the dark mode state instead of hard-coded `BLACK` and `WHITE`.

#### Scenario: Snake game in Day Mode
- **WHEN** Dark Mode is off and the snake game is launched
- **THEN** the game SHALL render with black elements on a white background

#### Scenario: Snake game in Dark Mode
- **WHEN** Dark Mode is on and the snake game is launched
- **THEN** the game SHALL render with white elements on a black background

### Requirement: Full refresh on mode change
The system SHALL perform a full display refresh (not a partial update) when Dark Mode is toggled, to avoid e-ink ghosting artifacts.

#### Scenario: Switching from Day to Dark Mode
- **WHEN** the user toggles Dark Mode on from the Settings screen
- **THEN** the screen SHALL re-render using `display->display()` (full refresh), not `partialUpdate()`

#### Scenario: Switching from Dark to Day Mode
- **WHEN** the user toggles Dark Mode off from the Settings screen
- **THEN** the screen SHALL re-render using `display->display()` (full refresh), not `partialUpdate()`
