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

#define XQUOTE(x) #x
#define QUOTE(x) XQUOTE(x)

#ifndef WIFI_SSID
#define WIFI_SSID "ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "password"
#endif

RichHttpServer<RichHttp::Generics::Configs::AsyncWebServer> server(80);

std::map<size_t, String> things;
size_t nextId = 1;

void handleGetThing(AsyncWebServerRequest* request, const UrlTokenBindings* bindings, RichHttp::Response& response) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    JsonObject thing = response.json.createNestedObject("thing");
    thing["id"] = id;
    thing["val"] = things[id];
  } else {
    response.setCode(404);
    response.json["error"] = "Not found";
  }
}

void handlePutThing(AsyncWebServerRequest* request, const UrlTokenBindings* bindings, JsonDocument& requestBody, RichHttp::Response& response) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    JsonObject req = requestBody.as<JsonObject>();
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

void handleDeleteThing(AsyncWebServerRequest* request, const UrlTokenBindings* bindings, RichHttp::Response& response) {
  size_t id = atoi(bindings->get("thing_id"));

  if (things.count(id)) {
    things.erase(id);
    response.json["success"] = true;
  } else {
    response.setCode(404);
    response.json["error"] = "Not found";
  }
}

void handleAbout(AsyncWebServerRequest* request, RichHttp::Response& response) {
  response.json["ip_address"] = WiFi.localIP().toString();
  response.json["free_heap"] = ESP.getFreeHeap();
}

void handleAddNewThing(AsyncWebServerRequest* request, JsonDocument& body, RichHttp::Response& response) {
  size_t id = nextId++;
  String val;

  if (! body["thing"].isNull()) {
    val = body["thing"]["val"].as<char*>();
  }

  things[id] = val;

  JsonObject obj = response.json.createNestedObject("thing");
  obj["id"] = id;
  obj["val"] = val;
}

void handleListThings(AsyncWebServerRequest* request, RichHttp::Response& response) {
  JsonArray arr = response.json.createNestedArray("things");

  for (std::map<size_t, String>::iterator it = things.begin(); it != things.end(); ++it) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = it->first;
    obj["val"] = it->second;
  }
}

void handleAuth(AsyncWebServerRequest* request, JsonDocument& body, RichHttp::Response& response) {
  JsonObject obj = body.as<JsonObject>();

  if (obj.containsKey("username") && obj.containsKey("password")) {
    server.requireAuthentication(obj["username"], obj["password"]);
  } else {
    server.disableAuthentication();
  }

  response.json["success"] = true;
}

void handleStaticResponse(AsyncWebServerRequest* request, const char* response) {
  request->send(200, "text/plain", response);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(QUOTE(WIFI_SSID), QUOTE(WIFI_PASSWORD));

  // Handle requests of the form GET /things/:thing_id.
  // For example, the request `GET /things/abc` would bind `abc` to the request
  // variable `thing_id`.
  server
    .buildHandler("/things/:thing_id")
    .onJson(HTTP_GET, std::bind(handleGetThing, _1, _2, _3))
    .onJsonBody(HTTP_PUT, std::bind(handlePutThing, _1, _2, _3, _4))
    .onJson(HTTP_DELETE, std::bind(handleDeleteThing, _1, _2, _3));

  server
    .buildHandler("/things")
    .onJsonBody(HTTP_POST, std::bind(handleAddNewThing, _1, _3, _4))
    .onJson(HTTP_GET, std::bind(handleListThings, _1, _3));

  server
    .buildHandler("/about")
    .setDisableAuthOverride()
    .onJson(HTTP_GET, std::bind(handleAbout, _1, _3));

  server
    .buildHandler("/sys/auth")
    .onJsonBody(HTTP_PUT, std::bind(handleAuth, _1, _3, _4));

  server.clearBuilders();

  server.begin();
}

void loop() {
}