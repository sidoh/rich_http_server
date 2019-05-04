#include <Arduino.h>
#include <ArduinoJson.h>

#include <UrlTokenBindings.h>
#include <RichHttpServer.h>
#include <map>
#include <functional>

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

#define XQUOTE(x) #x
#define QUOTE(x) XQUOTE(x)

#ifndef WIFI_SSID
#define WIFI_SSID ssid
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD password
#endif

#if defined(ARDUINO_ARCH_ESP8266)
RichHttpServer<RichHttp::Generics::Configs::ESP8266Config> server(80);
#else
RichHttpServer<RichHttp::Generics::Configs::ESP32Config> server(80);
#endif
std::map<size_t, String> things;
size_t nextId = 1;

void handleGetThing(const UrlTokenBindings* bindings, RichHttp::Response& response) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    JsonObject thing = response.json.createNestedObject("thing");
    thing["id"] = id;
    thing["val"] = things[id];

    // server.send(200, "application/json", things[id]);
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

void handlePutThing(const UrlTokenBindings* bindings, JsonDocument& request, RichHttp::Response& response) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    JsonObject req = request.as<JsonObject>();
    if (req.containsKey("thing")) {
      req = req["thing"];
      things[id] = req["val"].as<const char*>();
      response.json["success"] = true;
    } else {
      response.json["success"] = false;
      response.json["error"] = "Request object must contain key `thing'.";
      response.setCode(400);
    }
  } else {
    response.json["success"] = false;
    response.json["error"] = "Not found";
    response.setCode(404);
  }
}

void handleDeleteThing(const UrlTokenBindings* bindings, RichHttp::Response& response) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    things.erase(id);
    server.send(200, "application/json", "true");
    response.json["success"] = true;
  } else {
    response.json["success"] = false;
    response.json["error"] = "Not found";
    response.setCode(404);
  }
}

void handleAbout(RichHttp::Response& response) {
  response.json["ip_address"] = WiFi.localIP().toString();
  response.json["free_heap"] = ESP.getFreeHeap();
}

void handleAddNewThing() {
  size_t id = nextId++;
  const String& val = server.arg("plain");
  things[id] = val;

  StaticJsonDocument<100> doc;
  JsonObject obj = doc.createNestedObject("thing");
  obj["id"] = id;
  obj["val"] = val;

  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
}

void handleListThings() {
  StaticJsonDocument<1024> doc;
  JsonArray arr = doc.createNestedArray("things");

  for (std::map<size_t, String>::iterator it = things.begin(); it != things.end(); ++it) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = it->first;
    obj["val"] = it->second;
  }

  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
}

void handleAuth(JsonDocument& request, RichHttp::Response& response) {
  JsonObject obj = request.as<JsonObject>();

  if (obj.containsKey("username") && obj.containsKey("password")) {
    server.requireAuthentication(obj["username"], obj["password"]);
  } else {
    server.disableAuthentication();
  }

  response.json["success"] = true;
}

void handleStaticResponse(const char* response) {
  server.send(200, "text/plain", response);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(QUOTE(WIFI_SSID), QUOTE(WIFI_PASSWORD));

  // Handle requests of the form GET /things/:thing_id.
  // For example, the request `GET /things/abc` would bind `abc` to the request
  // variable `thing_id`.
  server
    .buildHandler("/things/:thing_id")
    .onJson(HTTP_GET, handleGetThing)
    .onJsonBody(HTTP_PUT, handlePutThing)
    .onJson(HTTP_DELETE, handleDeleteThing);

  server
    .buildHandler("/things")
    .onBody(HTTP_POST, std::bind(handleAddNewThing))
    .on(HTTP_GET, std::bind(handleListThings));

  server
    .buildHandler("/about")
    .setDisableAuthOverride()
    .onJson(HTTP_GET, std::bind(handleAbout, _2));

  server
    .buildHandler("/sys/auth")
    .onJsonBody(HTTP_PUT, std::bind(handleAuth, _2, _3));

  server.clearBuilders();
  server.begin();
}

void loop() {
  server.handleClient();
}