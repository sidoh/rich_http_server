#include "PlatformESP8266.h"
#include "PlatformESP32.h"

#if (defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)) && !defined(RICH_HTTP_ASYNC_WEBSERVER)

#if defined(ESP32)
#include <Update.h>
#else
#include <Updater.h>
#endif

using namespace RichHttp::Generics::Configs;

#if defined(ARDUINO_ARCH_ESP32)
using _Config = ESP32Config;
#else
using _Config = ESP8266Config;
#endif

using __fn_type = _Config::_fn_type;
using __context_type = _Config::_context_type;

const __fn_type _Config::OtaHandlerFn = [](__context_type context) {
  HTTPUpload& upload = context.server.upload();

  if (upload.status == UPLOAD_FILE_START) {
#if defined(ESP8266)
    WiFiUDP::stopAll();
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if(! Update.begin(maxSketchSpace)) {
      Update.printError(Serial);
      context.response.setCode(500);
      return;
    }
#else
    if(! Update.begin()) {
      Update.printError(Serial);
      context.response.setCode(500);
      return;
    }
#endif
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      context.response.setCode(500);
      return;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (! Update.end(true)) {
      Update.printError(Serial);
      context.response.setCode(500);
    }
  }
  yield();
};

const __fn_type _Config::OtaSuccessHandlerFn = [](__context_type context) {
  context.server.sendHeader("Connection", "close");
  context.server.sendHeader("Access-Control-Allow-Origin", "*");

  if (Update.hasError()) {
    context.response.json["success"] = false;
    context.response.setCode(500);
  } else {
    context.server.send(200, "text/plain", "Update successful");
  }

  delay(1000);

  ESP.restart();
};

#endif