## ADDED Requirements

### Requirement: Single-page web UI served from PROGMEM
The file manager web UI SHALL be a single self-contained HTML page with inline CSS and JavaScript, stored as a `PROGMEM` constant in a `WebUI.h` header file. It SHALL not depend on any external resources (CDNs, external stylesheets, or scripts).

#### Scenario: Page loads without internet
- **WHEN** a user connects to the ESP32 AP and navigates to `http://192.168.4.1/`
- **THEN** the web UI SHALL render completely using only embedded CSS and JS, with no external network requests

### Requirement: Directory browsing
The web UI SHALL display the contents of the current directory in a list or table view, showing each entry's name, type (file or folder), and size.

#### Scenario: Browse root directory on load
- **WHEN** the web UI loads in the browser
- **THEN** it SHALL fetch `/api/list?path=/` and display the root directory contents

#### Scenario: Navigate into a subdirectory
- **WHEN** the user clicks on a folder entry in the listing
- **THEN** the UI SHALL fetch the contents of that folder from `/api/list?path=<folder_path>` and display them

#### Scenario: Navigate back to parent directory
- **WHEN** the user is inside a subdirectory
- **THEN** the UI SHALL display a breadcrumb or back navigation element that navigates to the parent directory

#### Scenario: Display entry metadata
- **WHEN** the directory listing is displayed
- **THEN** each entry SHALL show the entry name, an icon or label indicating file vs folder, and the file size (formatted as KB/MB) for files

### Requirement: File upload via web UI
The web UI SHALL provide a mechanism to upload files to the currently browsed directory.

#### Scenario: Upload a single file
- **WHEN** the user selects a file using the upload control and confirms
- **THEN** the UI SHALL send a POST request to `/api/upload?path=<current_dir>` with the file as multipart form data
- **AND** the directory listing SHALL refresh after a successful upload

#### Scenario: Upload progress feedback
- **WHEN** a file upload is in progress
- **THEN** the UI SHALL display a visual indication that the upload is ongoing (e.g., a progress bar or spinner)

#### Scenario: Upload error feedback
- **WHEN** a file upload fails (server returns non-200 status)
- **THEN** the UI SHALL display an error message to the user

### Requirement: File and folder deletion via web UI
The web UI SHALL allow the user to delete files and folders from the current directory.

#### Scenario: Delete a file
- **WHEN** the user clicks the delete action on a file entry
- **THEN** the UI SHALL prompt for confirmation before sending a DELETE request to `/api/delete?path=<file_path>`
- **AND** the directory listing SHALL refresh after successful deletion

#### Scenario: Delete a folder
- **WHEN** the user clicks the delete action on a folder entry
- **THEN** the UI SHALL prompt for confirmation (warning that contents will be deleted) before sending the DELETE request
- **AND** the directory listing SHALL refresh after successful deletion

#### Scenario: Deletion confirmation prevents accidental deletion
- **WHEN** the user is prompted for deletion confirmation and cancels
- **THEN** no DELETE request SHALL be sent and the listing SHALL remain unchanged

### Requirement: File download via web UI
The web UI SHALL allow the user to download files.

#### Scenario: Download a file
- **WHEN** the user clicks the download action on a file entry
- **THEN** the browser SHALL initiate a file download from `/api/download?path=<file_path>`

### Requirement: Create new folder via web UI
The web UI SHALL allow the user to create a new folder in the current directory.

#### Scenario: Create folder
- **WHEN** the user activates the "new folder" action and enters a folder name
- **THEN** the UI SHALL send a POST request to `/api/mkdir?path=<current_dir>/<folder_name>`
- **AND** the directory listing SHALL refresh after successful creation

#### Scenario: Empty folder name rejected
- **WHEN** the user submits an empty or whitespace-only folder name
- **THEN** the UI SHALL show a validation error without sending a request

### Requirement: Rename files and folders via web UI
The web UI SHALL allow the user to rename files and folders.

#### Scenario: Rename a file
- **WHEN** the user activates the rename action on a file entry and enters a new name
- **THEN** the UI SHALL send a POST request to `/api/rename?path=<file_path>&newname=<new_name>`
- **AND** the directory listing SHALL refresh after successful rename

#### Scenario: Rename a folder
- **WHEN** the user activates the rename action on a folder entry and enters a new name
- **THEN** the UI SHALL send a POST request to `/api/rename?path=<folder_path>&newname=<new_name>`
- **AND** the directory listing SHALL refresh after successful rename

#### Scenario: Rename conflict feedback
- **WHEN** the rename API returns status `409` (name already exists)
- **THEN** the UI SHALL display an error message indicating the name is already taken

### Requirement: Responsive and usable UI
The web UI SHALL be usable on both desktop and mobile browsers, with a clean, minimal design.

#### Scenario: Desktop browser layout
- **WHEN** accessed from a desktop browser
- **THEN** the UI SHALL display the file listing in a readable table or list layout

#### Scenario: Mobile browser layout
- **WHEN** accessed from a mobile browser
- **THEN** the UI SHALL adapt its layout to be usable on smaller screens (responsive design)

### Requirement: Error state display
The web UI SHALL handle and display error states from API calls.

#### Scenario: API request fails
- **WHEN** any API request returns a non-200 status code
- **THEN** the UI SHALL parse the JSON error message and display it to the user

#### Scenario: Network error
- **WHEN** the connection to the ESP32 is lost during an operation
- **THEN** the UI SHALL display a connection error message
