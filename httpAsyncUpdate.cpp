#include "httpAsyncUpdate.h"

// flag to use from web update to reboot the ESP
bool shouldReboot = false;
char uName[32];
char pass[32];

void HttpAsyncUpdate::setup(AsyncWebServer* server, char* updatePath, const char* username, const char* password) {
    strcpy(uName,username);
    strcpy(pass,password);
  // Simple Firmware Update Form
  server->on(updatePath, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html",
                  "<form method='POST' action='/update' "
                  "enctype='multipart/form-data'><input type='file' "
                  "name='update'><input type='submit' value='Update'></form>");
  });
  server->on(
      updatePath, HTTP_POST,
      [](AsyncWebServerRequest* request) {
        shouldReboot = !Update.hasError();
        AsyncWebServerResponse* response = request->beginResponse(
            200, "text/plain", shouldReboot ? "OK" : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response);
      },
      [](AsyncWebServerRequest* request, String filename, size_t index,
         uint8_t* data, size_t len, bool final) {
        if (!request->authenticate(uName, pass)) {
          return request->requestAuthentication();
        }
        if (!index) {
          Serial.printf("Update Start: %s\n", filename.c_str());
          Update.runAsync(true);
          if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            Update.printError(Serial);
          }
        }
        if (!Update.hasError()) {
          if (Update.write(data, len) != len) {
            Update.printError(Serial);
          }
        }
        if (final) {
          if (Update.end(true)) {
            Serial.printf("Update Success: %uB\n", index + len);
          } else {
            Update.printError(Serial);
          }
        }
      });
}

void HttpAsyncUpdate::handle() {
  if (shouldReboot) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
}