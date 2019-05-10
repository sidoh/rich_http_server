#if defined(_ESPAsyncWebServer_H_) || defined(RICH_HTTP_ASYNC_WEBSERVER)
#include "PlatformAsyncWebServer.h"

#if defined(ESP8266)
#include <Updater.h>
#elif defined(ESP32)
#include <Update.h>
#endif

using namespace RichHttp::Generics;

using _Config = Configs::AsyncWebServer;

using __fn_type = _Config::_fn_type;
using __context_type = _Config::_context_type;

const __fn_type _Config::OtaHandlerFn = [](__context_type context) {
  if (context.upload.index == 0) {
    if (context.rawRequest->contentLength() > 0) {
      Update.begin(context.rawRequest->contentLength());
#if defined(ESP8266)
      Update.runAsync(true);
#endif
    }
  }

  if (Update.size() > 0) {
    if (Update.write(context.upload.data, context.upload.length) != context.upload.length) {
      Update.printError(Serial);

#if defined(ESP32)
      Update.abort();
#endif
    }

    if (context.upload.isFinal) {
      if (! Update.end(true)) {
        Update.printError(Serial);
#if defined(ESP32)
        Update.abort();
#endif
      }
    }
  }
};

const __fn_type _Config::OtaSuccessHandlerFn = [](__context_type context) {
  context.rawRequest->send(200, "text/plain", "success");

  delay(1000);

  ESP.restart();
};

#endif