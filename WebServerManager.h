#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include "Inkplate.h"
#include <WiFi.h>
#include <WebServer.h>
#include "SDHandler.h"

#define AP_SSID "FlickBook"
#define AP_PASSWORD "myflickbook"

class WebServerManager
{
public:
    WebServerManager(Inkplate *display);
    void start();
    void stop();
    void handleClient();
    bool isRunning();
    String getAPIP();
    String getWifiQRString();

private:
    Inkplate *display;
    WebServer server;
    bool running;

    String validatePath(const String &path);
    void handleRoot();
    void handleList();
    void handleDownload();
    void handleUpload();
    void handleUploadProcess();
    void handleDelete();
    void handleMkdir();
    void handleRename();
    void sendJsonError(int code, const String &message);
    void sendJsonSuccess(const String &message);

    SdFile uploadFile;
    String uploadTargetDir;
};

#endif
