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

  ; Use latest patch for given version
  RichHttpServer@~2.0.0

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

using namespace::placeholders;

// Define shorthand for common types
using RichHttpConfig = RichHttp::Generics::Configs::EspressifBuiltin;
using RequestContext = RichHttpConfig::RequestContextType;

// Use the default auth provider
SimpleAuthProvider authProvider;

// Use the builtin server (ESP8266WebServer for ESP8266, WebServer for ESP32).
// Listen on port 80.
RichHttpServer<RichHttpConfig> server(80, authProvider);

// Handlers for normal routes follow the same structure
void handleGetAbout(RequestContext& request) {
  // Respond with JSON body:
  //   {"message":"hello world"}
  request.response.json["message"] = "hello world";
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handleGetThing(RequestContext& request) {
  char buffer[100];
  sprintf("Thing ID is: %s\n", buffer, request.pathVariables.get("thing_id"));

  // Respond with JSON body:
  //   {"message":"Thing ID is..."}
  request.response.json["message"] = buffer;
}

// Handlers for routes with variables can be passed a `UrlTokenBindings` object
// which can be used to extract bindings to the variables
void handlePutThing(RequestContext& request) {
  JsonObject body = request.getJsonBody().as<JsonObject>();

  if (body.isNull()) {
    request.response.setCode(400);
    request.response.json["error"] = F("Invalid JSON.  Must be an object.");
    return;
  }

  char buffer[100];
  sprintf("Thing ID is: %s\nBody is: %s\n", request.pathVariables.get("thing_id"), request.getBody());

  request.response.json["message"] = buffer;
}

void setup() {
  // Create some routes
  server
    .buildHandler("/about")
    .on(HTTP_GET, handleGetAbout);

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

The only meaningful change to the above sketch for the builtin webserver is to use different generics:

```c++
#include <ESPAsyncWebServer.h>
#include <RichHttpServer.h>

using RichHttpConfig = RichHttp::Generics::Configs::AsyncWebServer;
using RequestContext = RichHttpConfig::RequestContextType;

// Use the default auth provider
SimpleAuthProvider authProvider;

// Use ESPAsyncWebServer
// Listen on port 80.
RichHttpServer<RichHttpConfig> server(80, authProvider);
```

The rest of the handlers are the same.  The raw `AsyncWebServerRequest` pointer is provided in the `RequestContext` parameter:

```c++
void myHandler(RequestContext& request) {
  AsyncWebServerRequest* rawRequest = request.rawRequest;
}
```

#### Authentication

The second argument to the `RichHttpServer` constructor is an `AuthProvider` reference.  `AuthProvider` has a simple interface:

```c++
  virtual bool isAuthenticationEnabled() const;

  virtual const String& getUsername() const;
  virtual const String& getPassword() const;
```

Other than the no-op implementation, in which authentication is always disabled, there are two additional provided implementations:

1. `BasicAuthProvider` exposes simple methods to enable/disable auth
2. `PassthroughAuthProvider` passes the interface methods onto a provided proxy object.  Useful if you've got a settings container that implements the above methods.

See the examples for further detail.

## Example projects

1. [esp8266_milight_hub](https://github.com/sidoh/esp8266_milight_hub)
2. [epaper_templates](https://github.com/sidoh/epaper_templates)
3. [esp8266_pin_server](https://github.com/sidoh/esp8266_pin_server)
4. [esp8266_thermometer](https://github.com/sidoh/esp8266_thermometer)

## Development

Build examples with, for instance:

```
platformio ci --board=d1_mini --lib=. examples/SimpleRestServer
```

#### New Release

1. Update version in `library.properties` and `library.json`.
2. Create a new tag with the corresponding version and push.