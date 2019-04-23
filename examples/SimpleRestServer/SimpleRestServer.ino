#include <Arduino.h>
#include <ArduinoJson.h>

#include <UrlTokenBindings.h>
#include <RichWebServer.h>
#include <map>

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

RichWebServer server(80);
std::map<size_t, String> things;
size_t nextId = 1;

void handleGetThing(const UrlTokenBindings* bindings) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    server.send(200, "application/json", things[id]);
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

void handlePutThing(const UrlTokenBindings* bindings) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    things[id] = server.arg("plain");
    server.send(200, "application/json", "true");
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

void handleDeleteThing(const UrlTokenBindings* bindings) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    things.erase(id);
    server.send(200, "application/json", "true");
  } else {
    server.send(404, "text/plain", "Not found");
  }
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

void handleAuth() {
  const String& val = server.arg("plain");

  StaticJsonDocument<100> doc;
  deserializeJson(doc, val);

  JsonObject obj = doc.as<JsonObject>();

  if (obj.containsKey("username") && obj.containsKey("password")) {
    server.requireAuthentication(obj["username"], obj["password"]);
  } else {
    server.disableAuthentication();
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Handle requests of the form GET /things/:thing_id.
  // For example, the request `GET /things/abc` would bind `abc` to the request
  // variable `thing_id`.
  server.onPatternAuthenticated("/things/:thing_id", HTTP_GET, handleGetThing);
  server.onPatternAuthenticated("/things/:thing_id", HTTP_PUT, handlePutThing);
  server.onPatternAuthenticated("/things/:thing_id", HTTP_DELETE, handleDeleteThing);
  server.onAuthenticated("/things", HTTP_POST, handleAddNewThing);
  server.onAuthenticated("/things", HTTP_GET, handleListThings);

  server.onAuthenticated("/sys/auth", HTTP_POST, handleAuth);

  server.begin();
}

void loop() {
  server.handleClient();
}