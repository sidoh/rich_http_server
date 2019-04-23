# Rich HTTP Server [![Build Status](https://travis-ci.org/sidoh/rich_http_server.svg?branch=master)](https://travis-ci.org/sidoh/rich_http_server)

### Examples

Examples are found in the `./examples` directory.  This is the easiest way to get started.

## Compatible Hardware

This library is built on top of handler bindings tied to the espressif platform.  It's an addon for `ESP8266WebServer`, which works on ESP8266 and ESP32.

## Development

Build examples with, for instance:

```
platformio ci --board=d1_mini --lib=. examples/esp8266_simple
```

#### New Release

1. Update version in `library.properties` and `library.json`.
1. Create a new tag with the corresponding version and push.