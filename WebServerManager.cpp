#include "WebServerManager.h"

extern SDHandler sdHandler;

WebServerManager::WebServerManager(Inkplate *display) : display(display), server(80), running(false) {}

void WebServerManager::start()
{
    if (running)
        return;

    WiFi.softAP("FlickBook", "myflickbook");
    Serial.println("WiFi AP started. IP: " + WiFi.softAPIP().toString());

    server.on("/", HTTP_GET, [this]()
              { handleRoot(); });
    server.on("/api/list", HTTP_GET, [this]()
              { handleList(); });
    server.on("/api/download", HTTP_GET, [this]()
              { handleDownload(); });
    server.on("/api/upload", HTTP_POST, [this]()
              { handleUpload(); }, [this]()
              { handleUploadProcess(); });
    server.on("/api/delete", HTTP_DELETE, [this]()
              { handleDelete(); });
    server.on("/api/mkdir", HTTP_POST, [this]()
              { handleMkdir(); });
    server.on("/api/rename", HTTP_POST, [this]()
              { handleRename(); });

    server.begin();
    running = true;
    Serial.println("Web server started on port 80");
}

void WebServerManager::stop()
{
    if (!running)
        return;

    server.stop();
    WiFi.softAPdisconnect(true);
    running = false;
    Serial.println("Web server and WiFi AP stopped");
}

String WebServerManager::getAPIP()
{
    return WiFi.softAPIP().toString();
}

void WebServerManager::handleClient()
{
    if (!running)
        return;
    server.handleClient();
}

bool WebServerManager::isRunning()
{
    return running;
}

// --- Path validation ---

String WebServerManager::validatePath(const String &path)
{
    if (path.indexOf("..") >= 0)
    {
        return "";
    }
    String normalized = sdHandler.normalizePath(path);
    return normalized;
}

// --- JSON helpers ---

void WebServerManager::sendJsonError(int code, const String &message)
{
    String json = "{\"error\":\"" + message + "\"}";
    server.send(code, "application/json", json);
}

void WebServerManager::sendJsonSuccess(const String &message)
{
    String json = "{\"success\":\"" + message + "\"}";
    server.send(200, "application/json", json);
}

// --- Route handlers ---

void WebServerManager::handleRoot()
{
    SdFile file;
    if (!sdHandler.openFileForRead("/assets/webserver/index.html", file))
    {
        sendJsonError(500, "Web UI file not found on SD card");
        return;
    }

    uint32_t fileSize = file.fileSize();
    server.setContentLength(fileSize);
    server.send(200, "text/html", "");

    const size_t CHUNK_SIZE = 1024;
    uint8_t buf[CHUNK_SIZE];
    uint32_t remaining = fileSize;
    while (remaining > 0)
    {
        size_t toRead = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
        int bytesRead = file.read(buf, toRead);
        if (bytesRead <= 0)
            break;
        server.client().write(buf, bytesRead);
        remaining -= bytesRead;
    }
    file.close();
}

void WebServerManager::handleList()
{
    String path = server.hasArg("path") ? server.arg("path") : "/";
    String validPath = validatePath(path);
    if (validPath.isEmpty())
    {
        sendJsonError(400, "Invalid path");
        return;
    }

    if (!sdHandler.folderExists(validPath))
    {
        sendJsonError(404, "Directory not found");
        return;
    }

    std::vector<FileEntry> entries = sdHandler.listFilesWithMeta(validPath);

    String json = "[";
    for (size_t i = 0; i < entries.size(); i++)
    {
        if (i > 0)
            json += ",";
        json += "{\"name\":\"";
        // Escape quotes in filenames
        String escapedName = entries[i].name;
        escapedName.replace("\\", "\\\\");
        escapedName.replace("\"", "\\\"");
        json += escapedName;
        json += "\",\"isDir\":";
        json += entries[i].isDir ? "true" : "false";
        json += ",\"size\":";
        json += String(entries[i].size);
        json += "}";
    }
    json += "]";

    server.send(200, "application/json", json);
}

void WebServerManager::handleDownload()
{
    String path = server.hasArg("path") ? server.arg("path") : "";
    String validPath = validatePath(path);
    if (validPath.isEmpty())
    {
        sendJsonError(400, "Invalid path");
        return;
    }

    if (!sdHandler.fileExists(validPath))
    {
        sendJsonError(404, "File not found");
        return;
    }

    if (sdHandler.folderExists(validPath))
    {
        // Check if it's actually a directory
        SdFile testFile;
        if (sdHandler.openFileForRead(validPath, testFile))
        {
            if (testFile.isDir())
            {
                testFile.close();
                sendJsonError(400, "Cannot download a directory");
                return;
            }
            testFile.close();
        }
    }

    SdFile file;
    if (!sdHandler.openFileForRead(validPath, file))
    {
        sendJsonError(500, "Failed to open file");
        return;
    }

    if (file.isDir())
    {
        file.close();
        sendJsonError(400, "Cannot download a directory");
        return;
    }

    uint32_t fileSize = file.fileSize();

    // Extract filename from path
    int lastSlash = validPath.lastIndexOf('/');
    String fileName = (lastSlash >= 0) ? validPath.substring(lastSlash + 1) : validPath;

    server.sendHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
    server.setContentLength(fileSize);
    server.send(200, "application/octet-stream", "");

    // Stream file in chunks
    const size_t CHUNK_SIZE = 1024;
    uint8_t buf[CHUNK_SIZE];
    uint32_t remaining = fileSize;
    while (remaining > 0)
    {
        size_t toRead = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
        int bytesRead = file.read(buf, toRead);
        if (bytesRead <= 0)
            break;
        server.client().write(buf, bytesRead);
        remaining -= bytesRead;
    }
    file.close();
}

void WebServerManager::handleUpload()
{
    sendJsonSuccess("File uploaded");
}

void WebServerManager::handleUploadProcess()
{
    HTTPUpload &upload = server.upload();
    String path = server.hasArg("path") ? server.arg("path") : "/";
    String validPath = validatePath(path);

    if (upload.status == UPLOAD_FILE_START)
    {
        if (validPath.isEmpty())
        {
            return;
        }
        // Ensure target directory exists
        sdHandler.createFolderRecursive(validPath);

        String fullPath = validPath;
        if (!fullPath.endsWith("/"))
            fullPath += "/";
        fullPath += upload.filename;

        Serial.println("Upload start: " + fullPath);
        uploadFile.open(fullPath.c_str(), O_WRITE | O_CREAT | O_TRUNC);
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (uploadFile.isOpen())
        {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (uploadFile.isOpen())
        {
            uploadFile.close();
            Serial.println("Upload complete: " + String(upload.totalSize) + " bytes");
        }
    }
}

void WebServerManager::handleDelete()
{
    String path = server.hasArg("path") ? server.arg("path") : "";
    String validPath = validatePath(path);
    if (validPath.isEmpty())
    {
        sendJsonError(400, "Invalid path");
        return;
    }

    if (!sdHandler.fileExists(validPath))
    {
        sendJsonError(404, "Path not found");
        return;
    }

    if (sdHandler.deletePath(validPath))
    {
        sendJsonSuccess("Deleted");
    }
    else
    {
        sendJsonError(500, "Failed to delete");
    }
}

void WebServerManager::handleMkdir()
{
    String path = server.hasArg("path") ? server.arg("path") : "";
    String validPath = validatePath(path);
    if (validPath.isEmpty())
    {
        sendJsonError(400, "Invalid path");
        return;
    }

    if (sdHandler.createFolderRecursive(validPath))
    {
        sendJsonSuccess("Directory created");
    }
    else
    {
        sendJsonError(500, "Failed to create directory");
    }
}

void WebServerManager::handleRename()
{
    String path = server.hasArg("path") ? server.arg("path") : "";
    String newname = server.hasArg("newname") ? server.arg("newname") : "";

    String validPath = validatePath(path);
    if (validPath.isEmpty() || newname.isEmpty())
    {
        sendJsonError(400, "Invalid path or new name");
        return;
    }

    if (newname.indexOf("..") >= 0 || newname.indexOf('/') >= 0)
    {
        sendJsonError(400, "Invalid new name");
        return;
    }

    if (!sdHandler.fileExists(validPath))
    {
        sendJsonError(404, "Path not found");
        return;
    }

    // Build new full path: same parent directory + new name
    int lastSlash = validPath.lastIndexOf('/');
    String parentDir = (lastSlash > 0) ? validPath.substring(0, lastSlash) : "/";
    String newFullPath = parentDir + "/" + newname;

    if (sdHandler.fileExists(newFullPath))
    {
        sendJsonError(409, "A file or folder with that name already exists");
        return;
    }

    if (sdHandler.renamePath(validPath, newFullPath))
    {
        sendJsonSuccess("Renamed");
    }
    else
    {
        sendJsonError(500, "Failed to rename");
    }
}
