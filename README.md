# Rich HTTP Server [![Build Status](https://travis-ci.org/sidoh/rich_http_server.svg?branch=master)](https://travis-ci.org/sidoh/rich_http_server)

This is a library that's built on top of common HTTP servers for Espressif (ESP8266, ESP32) platforms.  It makes it simple to build modern REST APIs, and removes some of the fuss from implementing common features such as authentication.

### Examples

Examples are found in the `./examples` directory.  This is the easiest way to get started.

## Compatible Hardware

This library is built on top of handler bindings tied to the espressif platform.  It's an addon for `ESP8266WebServer`, which works on ESP8266 and ESP32.  It is also compatible with [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) on both the ESP8266 and ESP32.

## Installation

### PlatformIO

RichHttpServer is registered with the PlatformIO library registry.  To use it, simply add it to your dependencies in your project's `platformio.ini`.  For example:

```ini
[env:myboard]
platform = espressif8266
board = d1_mini
framework = arduino
lib_deps =
  ; Use most recent published version
  RichHttpServer

  ; Use branch from github
  ; sidoh/rich_http_server#branch
```

## Design

The base object the user interacts with is `RichHttpServer`.  It is templated with a configuration which integrates it with the HTTP server of your choice.  The resulting class will _extend_ the given HTTP library, so it's easy to drop into existing projects, and does not require that you learn a bunch of new interfaces to get started.

## Getting Started

#### Builtin server

```c++
#include <RichHttpServer.h>
#include <functional>

// Use the builtin server (ESP8266WebServer for ESP8266, WebServer for ESP32).
// Listen on port 80.
RichHttpServer<RichHttp::Configs::Generics::EspressifBuiltin> server(80);

using namespace::placeholders;

// Handlers for normal routes follow the same structure
void handleGetAbout() {
  server.send(200, "text/plain", "hello world");
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handleGetThing(const UrlTokenBindings* bindings) {
  String response = "thing ID = " + String(bindings.get("thing_id"));
  server.send(200, "text/plain", response);
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handlePutThing(const UrlTokenBindings* bindings) {
  String body = server.args("plain");
  String response = "thing ID = " + String(bindings.get("thing_id")) + ", body was = " + body;

  server.send(200, "text/plain", response);
}

void setup() {
  // Create some routes
  server
    .buildHandler("/about")
    // std::bind removes the need to take in a dummy UrlTokenBindings parameter
    .on(HTTP_GET, std::bind(handleGetAbout));

  server
    .buildHandler("/things/:thing_id")
    .on(HTTP_GET, std::bind(handleGetThing, _1))
    .on(HTTP_PUT, std::bind(handlePutThing, _1));

  // Builders are cached and should be cleared after we're finished
  server.clearBuilders();

  server.begin();
}

void loop() {
  server.handleClient();
}
```

#### AsyncWebServer

Note that you may need to define the compiler flag `RICH_HTTP_ASYNC_WEBSERVER` to prevent conflicts between the builtin and async servers (they both define globals with the same name).

```c++
#include <ESPAsyncWebServer.h>
#include <RichHttpServer.h>

// Use ESPAsyncWebServer
// Listen on port 80.
RichHttpServer<RichHttp::Configs::Generics::AsyncWebServer> server(80);

using namespace::placeholders;

// Handlers for normal routes follow the same structure
void handleGetAbout(AsyncWebServerRequest* request) {
  server.send(200, "text/plain", "hello world");
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handleGetThing(AsyncWebServerRequest* request, const UrlTokenBindings* bindings) {
  String response = "thing ID = " + String(bindings.get("thing_id"));
  server.send(200, "text/plain", response);
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handlePutThing(AsyncWebServerRequest* request, const UrlTokenBindings* bindings, uint8_t* data, size_t len, size_t index, size_t total) {
  String body(reinterpret_cast<char*>(data));
  String response = "thing ID = " + String(bindings.get("thing_id")) + ", body was = " + body;

  server.send(200, "text/plain", response);
}

void setup() {
  // Create some routes
  server
    .buildHandler("/about")
    .on(HTTP_GET, std::bind(handleGetAbout, _1));

  server
    .buildHandler("/things/:thing_id")
    .on(HTTP_GET, std::bind(handleGetThing, _1, _2))
    .on(HTTP_PUT, std::bind(handlePutThing, _1, _2, _3, _4, _5));

  // Builders are cached and should be cleared after we're finished
  server.clearBuilders();

  server.begin();
}

void loop() {
  server.handleClient();
}
```

## Development

Build examples with, for instance:

```
platformio ci --board=d1_mini --lib=. examples/SimpleRestServer
```

#### New Release

1. Update version in `library.properties` and `library.json`.
1. Create a new tag with the corresponding version and push.