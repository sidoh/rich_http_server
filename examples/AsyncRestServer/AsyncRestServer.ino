#include <Arduino.h>
#include <ArduinoJson.h>

#include <ESPAsyncWebServer.h>

#include <UrlTokenBindings.h>
#include <RichHttpServer.h>
#include <map>

using namespace std::placeholders;

// Simple REST server with CRUD routes:
//
//   * POST /things - add a new thing
//   * GET  /things - list all things
//   * GET  /things/:id - get specified thing
//   * PUT  /things/:id - update value for specified thing
//   * DELETE /things/:id - delete specified thing
//
// Also supports a route to enable/disable password authentication:
//
//   POST /sys/auth
//   Expects a JSON body with "username" and "password" keys.
//
// Note that you'll need to have the ArduinoJson library installed as well.

const char* WIFI_SSID = "ssid";
const char* WIFI_PASSWORD = "password";

RichHttpServer<RichHttp::Generics::Configs::AsyncWebServer> server(80);

std::map<size_t, String> things;
size_t nextId = 1;

void handleGetThing(AsyncWebServerRequest* request, const UrlTokenBindings* bindings) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    request->send(200, "application/json", things[id]);
  } else {
    request->send(404, "text/plain", "Not found");
  }
}

void handlePutThing(AsyncWebServerRequest* request, const UrlTokenBindings* bindings, uint8_t* data, size_t len, size_t index, size_t total) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    things[id] = String(reinterpret_cast<char*>(data));
    request->send(200, "application/json", "true");
  } else {
    request->send(404, "text/plain", "Not found");
  }
}

void handleDeleteThing(AsyncWebServerRequest* request, const UrlTokenBindings* bindings) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    things.erase(id);
    request->send(200, "application/json", "true");
  } else {
    request->send(404, "text/plain", "Not found");
  }
}

void handleAbout(AsyncWebServerRequest* request) {
  request->send(200, "text/plain", "about");
}

void handleAddNewThing(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
  size_t id = nextId++;
  const String& val = String(reinterpret_cast<char*>(data));
  things[id] = val;

  StaticJsonDocument<100> doc;
  JsonObject obj = doc.createNestedObject("thing");
  obj["id"] = id;
  obj["val"] = val;

  String response;
  serializeJson(doc, response);

  request->send(200, "application/json", response);
}

void handleListThings(AsyncWebServerRequest* request) {
  StaticJsonDocument<1024> doc;
  JsonArray arr = doc.createNestedArray("things");

  for (std::map<size_t, String>::iterator it = things.begin(); it != things.end(); ++it) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = it->first;
    obj["val"] = it->second;
  }

  String response;
  serializeJson(doc, response);

  request->send(200, "application/json", response);
}

void handleAuth(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
  const String val(reinterpret_cast<char*>(data));

  StaticJsonDocument<100> doc;
  deserializeJson(doc, val);

  JsonObject obj = doc.as<JsonObject>();

  if (obj.containsKey("username") && obj.containsKey("password")) {
    server.requireAuthentication(obj["username"], obj["password"]);
  } else {
    server.disableAuthentication();
  }

  request->send(200, "text/plain", "OK");
}

void handleStaticResponse(AsyncWebServerRequest* request, const char* response) {
  request->send(200, "text/plain", response);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Handle requests of the form GET /things/:thing_id.
  // For example, the request `GET /things/abc` would bind `abc` to the request
  // variable `thing_id`.
  server
    .buildHandler("/things/:thing_id")
    .on(HTTP_GET, std::bind(handleGetThing, _1, _2))
    .on(HTTP_PUT, std::bind(handlePutThing, _1, _2))
    .on(HTTP_DELETE, std::bind(handleDeleteThing, _1, _2));

  server
    .buildHandler("/things")
    .onBody(HTTP_POST, std::bind(handleAddNewThing, _1, _3, _4, _5, _6))
    .on(HTTP_GET, std::bind(handleListThings, _1));

  server
    .buildHandler("/about")
    .setDisableAuthOverride()
    .on(HTTP_GET, std::bind(handleAbout, _1));

  server.begin();
}

void loop() {
}