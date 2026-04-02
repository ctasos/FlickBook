## ADDED Requirements

### Requirement: Serve web UI from root endpoint
The HTTP server SHALL serve the embedded web UI HTML page when a GET request is made to `/`. The response SHALL have content type `text/html` and status `200`.

#### Scenario: Browser loads web UI
- **WHEN** a connected client navigates to `http://192.168.4.1/` in a browser
- **THEN** the server SHALL respond with the full HTML page containing the file manager web UI

### Requirement: List directory contents via API
The HTTP server SHALL expose a `GET /api/list` endpoint that accepts a `path` query parameter and returns a JSON array of directory entries.

#### Scenario: List root directory
- **WHEN** a GET request is made to `/api/list?path=/`
- **THEN** the server SHALL respond with a JSON array where each entry contains `"name"` (string), `"isDir"` (boolean), and `"size"` (number, in bytes, 0 for directories)
- **AND** the response content type SHALL be `application/json`

#### Scenario: List subdirectory
- **WHEN** a GET request is made to `/api/list?path=/books`
- **THEN** the server SHALL respond with the contents of the `/books` directory in the same JSON format

#### Scenario: List non-existent directory
- **WHEN** a GET request is made to `/api/list?path=/nonexistent`
- **THEN** the server SHALL respond with status `404` and a JSON error message

#### Scenario: Path parameter missing
- **WHEN** a GET request is made to `/api/list` without a `path` parameter
- **THEN** the server SHALL default to listing the root directory `/`

### Requirement: Download files via API
The HTTP server SHALL expose a `GET /api/download` endpoint that streams a file from the SD card to the client.

#### Scenario: Download an existing file
- **WHEN** a GET request is made to `/api/download?path=/books/mybook.epub`
- **THEN** the server SHALL stream the file content with content type `application/octet-stream` and a `Content-Disposition` header with the filename
- **AND** the file SHALL be streamed in chunks to avoid loading the entire file into RAM

#### Scenario: Download non-existent file
- **WHEN** a GET request is made to `/api/download?path=/nonexistent.txt`
- **THEN** the server SHALL respond with status `404` and a JSON error message

#### Scenario: Attempt to download a directory
- **WHEN** a GET request is made to `/api/download?path=/books`
- **THEN** the server SHALL respond with status `400` and a JSON error message

### Requirement: Upload files via API
The HTTP server SHALL expose a `POST /api/upload` endpoint that accepts a multipart file upload and saves it to the specified directory on the SD card.

#### Scenario: Upload a file to a directory
- **WHEN** a POST request is made to `/api/upload?path=/books` with a multipart file body
- **THEN** the server SHALL save the uploaded file to `/books/<filename>` on the SD card
- **AND** the server SHALL respond with status `200` and a JSON success message
- **AND** the file SHALL be written in chunks to avoid loading the entire upload into RAM

#### Scenario: Upload to non-existent directory
- **WHEN** a POST request is made to `/api/upload?path=/newdir` and `/newdir` does not exist
- **THEN** the server SHALL create the directory recursively and save the file

#### Scenario: Upload overwrites existing file
- **WHEN** a file with the same name already exists at the target path
- **THEN** the server SHALL overwrite the existing file with the uploaded content

### Requirement: Delete files and folders via API
The HTTP server SHALL expose a `DELETE /api/delete` endpoint that removes a file or empty folder from the SD card.

#### Scenario: Delete an existing file
- **WHEN** a DELETE request is made to `/api/delete?path=/books/old.epub`
- **THEN** the server SHALL delete the file from the SD card and respond with status `200` and a JSON success message

#### Scenario: Delete an empty directory
- **WHEN** a DELETE request is made to `/api/delete?path=/emptydir`
- **THEN** the server SHALL remove the empty directory and respond with status `200`

#### Scenario: Delete a non-empty directory
- **WHEN** a DELETE request is made to `/api/delete?path=/books` and the directory contains files
- **THEN** the server SHALL recursively delete all contents and then the directory itself, responding with status `200`

#### Scenario: Delete non-existent path
- **WHEN** a DELETE request is made to `/api/delete?path=/nonexistent`
- **THEN** the server SHALL respond with status `404` and a JSON error message

### Requirement: Create directories via API
The HTTP server SHALL expose a `POST /api/mkdir` endpoint that creates a new directory on the SD card.

#### Scenario: Create a new directory
- **WHEN** a POST request is made to `/api/mkdir?path=/newdir`
- **THEN** the server SHALL create the directory and respond with status `200` and a JSON success message

#### Scenario: Create nested directories
- **WHEN** a POST request is made to `/api/mkdir?path=/a/b/c` and intermediate directories do not exist
- **THEN** the server SHALL create all intermediate directories recursively

#### Scenario: Create directory that already exists
- **WHEN** a POST request is made to `/api/mkdir?path=/books` and the directory already exists
- **THEN** the server SHALL respond with status `200` (idempotent, no error)

### Requirement: Rename files and folders via API
The HTTP server SHALL expose a `POST /api/rename` endpoint that renames a file or folder on the SD card.

#### Scenario: Rename an existing file
- **WHEN** a POST request is made to `/api/rename?path=/books/old.epub&newname=new.epub`
- **THEN** the server SHALL rename the file to `/books/new.epub` and respond with status `200` and a JSON success message

#### Scenario: Rename an existing directory
- **WHEN** a POST request is made to `/api/rename?path=/olddir&newname=newdir`
- **THEN** the server SHALL rename the directory and respond with status `200`

#### Scenario: Rename non-existent path
- **WHEN** a POST request is made to `/api/rename?path=/nonexistent&newname=new`
- **THEN** the server SHALL respond with status `404` and a JSON error message

#### Scenario: Rename to a name that already exists
- **WHEN** a POST request is made to `/api/rename` and the target name already exists at the same level
- **THEN** the server SHALL respond with status `409` and a JSON error message

### Requirement: Path traversal prevention
All API endpoints SHALL validate the `path` parameter to prevent path traversal attacks. Paths containing `..` segments SHALL be rejected.

#### Scenario: Path with traversal attempt
- **WHEN** any API request includes a `path` parameter containing `..` (e.g., `/api/list?path=/../etc`)
- **THEN** the server SHALL respond with status `400` and a JSON error message
- **AND** the request SHALL NOT access any files outside the SD card root

#### Scenario: Paths are normalized
- **WHEN** a path parameter contains double slashes or trailing slashes (e.g., `//books/` or `/books//`)
- **THEN** the server SHALL normalize the path before processing

### Requirement: SDHandler enhancements for web server
`SDHandler` SHALL be enhanced with additional methods needed by the web server endpoints that are not currently available.

#### Scenario: Directory listing with metadata
- **WHEN** the web server needs to list directory entries with type (file vs directory) and size
- **THEN** `SDHandler` SHALL provide a method that returns entries with name, isDir, and size information

#### Scenario: File deletion
- **WHEN** the web server needs to delete a file or directory
- **THEN** `SDHandler` SHALL provide a `deleteFile(path)` method and a `deleteDir(path)` method (or a combined `deletePath(path)` method)

#### Scenario: File rename
- **WHEN** the web server needs to rename a file or directory
- **THEN** `SDHandler` SHALL provide a `renameFile(oldPath, newPath)` method

#### Scenario: Streaming file read
- **WHEN** the web server needs to stream a file to a client without loading it into RAM
- **THEN** `SDHandler` SHALL provide access to chunked file reading (e.g., a method that opens a file and reads blocks into a provided buffer)
