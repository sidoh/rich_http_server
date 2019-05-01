#pragma once

#include "Generics.h"
#include "PlatformEspressif.h"

#include <functional>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WebServer.h>

namespace RichHttp {
  namespace Generics {
    class ESP8266RequestHandler;

    namespace Configs {
      using ESP8266Config = espressif_config<ESP8266WebServer, ESP8266RequestHandler>;
    };

    class ESP8266RequestHandler : public EspressifRequestHandler<Configs::ESP8266Config> {
      public:
        ESP8266RequestHandler(
          typename Configs::ESP8266Config::HttpMethod method,
          const char* path,
          typename Configs::ESP8266Config::RequestHandlerFn::type handlerFn,
          typename Configs::ESP8266Config::BodyRequestHandlerFn::type bodyFn = NULL,
          typename Configs::ESP8266Config::UploadRequestHandlerFn::type uploadFn = NULL
        ) : EspressifRequestHandler<Configs::ESP8266Config>(method, path, handlerFn, bodyFn, uploadFn)
        { }
    };
  };
};
#endif