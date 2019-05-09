#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#if defined(ESP32)
#include <SPIFFS.h>
#endif

#include <map>
#include <functional>

#include <UrlTokenBindings.h>
#include <RichHttpServer.h>

using namespace std::placeholders;

// Simple REST server with CRUD routes:
//
//   * POST /things - add a new thing
//   * GET  /things - list all things
//   * GET  /things/:id - get specified thing
//   * PUT  /things/:id - update value for specified thing
//   * DELETE /things/:id - delete specified thing
//
//   * GET  /files - list all files
//   * POST /files/:filename - create new file with name
//   * GET  /files/:filename - read given file
//   * DELETE /files/:filename - delete given file
//
// Also supports a route to enable/disable password authentication:
//
//   POST /sys/auth
//   Expects a JSON body with "username" and "password" keys.
//
// Note that you'll need to have the ArduinoJson library installed as well.

#define XQUOTE(x) #x
#define QUOTE(x) XQUOTE(x)

#ifndef WIFI_SSID
#define WIFI_SSID ssid
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD password
#endif

using RichHttpConfig = RichHttp::Generics::Configs::EspressifBuiltin;
using RequestContext = RichHttpConfig::RequestContextType;

SimpleAuthProvider authProvider;
RichHttpServer<RichHttpConfig> server(80, authProvider);

std::map<size_t, String> things;
size_t nextId = 1;

void handleGetThing(RequestContext& requestContext) {
  size_t id = atoi(requestContext.pathVariables.get("thing_id"));

  if (things.count(id)) {
    JsonObject thing = requestContext.response.json.createNestedObject("thing");
    thing["id"] = id;
    thing["val"] = things[id];
  } else {
    requestContext.response.setCode(404);
    requestContext.response.json["error"] = "Not found";
  }
}

void handlePutThing(RequestContext& request) {
  size_t id = atoi(request.pathVariables.get("thing_id"));

  if (things.count(id)) {
    JsonObject req = request.getJsonBody().as<JsonObject>();

    if (req.containsKey("thing")) {
      req = req["thing"];
      things[id] = req["val"].as<const char*>();
      request.response.json["success"] = true;
    } else {
      request.response.json["success"] = false;
      request.response.json["error"] = "Request object must contain key `thing'.";
      request.response.setCode(400);
    }
  } else {
    request.response.json["success"] = false;
    request.response.json["error"] = "Not found";
    request.response.setCode(404);
  }
}

void handleDeleteThing(RequestContext& request) {
  size_t id = atoi(request.pathVariables.get("thing_id"));

  if (things.count(id)) {
    things.erase(id);
    request.response.json["success"] = true;
  } else {
    request.response.json["success"] = false;
    request.response.json["error"] = "Not found";
    request.response.setCode(404);
  }
}

void handleAbout(RequestContext& request) {
  request.response.json["ip_address"] = WiFi.localIP().toString();
  request.response.json["free_heap"] = ESP.getFreeHeap();
  request.response.json["version"] = "builtin";
}

void handleAddNewThing(RequestContext& request) {
  JsonObject body = request.getJsonBody().as<JsonObject>();

  if (! body["thing"].isNull()) {
    size_t id = nextId++;
    things[id] = body["thing"]["val"].as<char*>();

    JsonObject obj = request.response.json.createNestedObject("thing");
    obj["id"] = id;
    obj["val"] = things[id];
  } else {
    request.response.setCode(400);
    request.response.json["error"] = "Must contain key `thing'";
  }
}

void handleListThings(RequestContext& request) {
  JsonArray arr = request.response.json.createNestedArray("things");

  for (std::map<size_t, String>::iterator it = things.begin(); it != things.end(); ++it) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = it->first;
    obj["val"] = it->second;
  }
}

void handleAuth(RequestContext& request) {
  JsonObject obj = request.getJsonBody().as<JsonObject>();

  if (obj.containsKey("username") && obj.containsKey("password")) {
    authProvider.requireAuthentication(obj["username"], obj["password"]);
  } else {
    authProvider.disableAuthentication();
  }

  request.response.json["success"] = true;
}

void handleListFiles(RequestContext& request) {
  JsonArray files = request.response.json.to<JsonArray>();

#if defined(ESP8266)
  Dir dir = SPIFFS.openDir("/files/");

  while (dir.next()) {
    JsonObject file = files.createNestedObject();
    file["name"] = dir.fileName();
    file["size"] = dir.fileSize();
  }
#elif defined(ESP32)
  File dir = SPIFFS.open("/files/");

  if (!dir || !dir.isDirectory()) {
    Serial.print(F("Path is not a directory"));

    request.response.setCode(500);
    request.response.json["error"] = F("Expected path to be a directory, but wasn't");
    return;
  }

  while (File dirFile = dir.openNextFile()) {
    JsonObject file = files.createNestedObject();

    file["name"] = String(dirFile.name());
    file["size"] = dirFile.size();
  }
#endif
}

void handleReadFile(RequestContext& request) {
  String filename = String("/files/") + request.pathVariables.get("filename");

  if (SPIFFS.exists(filename)) {
    File f = SPIFFS.open(filename, "r");
    server.streamFile(f, "text/plain");
  } else {
    request.response.json["error"] = "Not found";
    request.response.setCode(404);
  }
}

void handleAddNewFile(RequestContext& request) {
  request.response.json["success"] = true;
}

void handleAddFileUpload(RequestContext& request) {
  HTTPUpload& upload = server.upload();
  static File updateFile;
  String filename = String("/files/") + request.pathVariables.get("filename");

  if (upload.status == UPLOAD_FILE_START) {
    updateFile = SPIFFS.open(filename, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (updateFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println(F("Error updating web file"));
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    updateFile.close();
  }
}

void handleDeleteFile(RequestContext& request) {
  String filename = String("/files/") + request.pathVariables.get("filename");

  if (SPIFFS.exists(filename)) {
    SPIFFS.remove(filename);
    request.response.json["success"] = true;
  } else {
    request.response.setCode(404);
    request.response.json["error"] = "Not found";
  }
}

void handleStaticResponse(const char* response) {
  server.send(200, "text/plain", response);
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();

  WiFi.begin(QUOTE(WIFI_SSID), QUOTE(WIFI_PASSWORD));

  // Handle requests of the form GET /things/:thing_id.
  // For example, the request `GET /things/abc` would bind `abc` to the request
  // variable `thing_id`.
  server
    .buildHandler("/things/:thing_id")
    .on(HTTP_GET, handleGetThing)
    .on(HTTP_PUT, handlePutThing)
    .on(HTTP_DELETE, handleDeleteThing);

  server
    .buildHandler("/things")
    .on(HTTP_POST, handleAddNewThing)
    .on(HTTP_GET, handleListThings);

  server
    .buildHandler("/about")
    .setDisableAuthOverride()
    .on(HTTP_GET, handleAbout);

  server
    .buildHandler("/sys/auth")
    .on(HTTP_PUT, handleAuth);

  server
    .buildHandler("/files")
    .on(HTTP_GET, handleListFiles);

  server
    .buildHandler("/files/:filename")
    .on(HTTP_DELETE, handleDeleteFile)
    .on(HTTP_GET, handleReadFile)
    .on(HTTP_POST, handleAddNewFile, handleAddFileUpload);

  server
    .buildHandler("/firmware")
    .handleOTA();

  server.clearBuilders();
  server.begin();
}

void loop() {
  server.handleClient();
}