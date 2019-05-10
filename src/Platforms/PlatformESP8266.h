#pragma once

#include "Generics.h"
#include "PlatformEspressif.h"

#include <functional>

#if defined(ARDUINO_ARCH_ESP8266) && !defined(RICH_HTTP_ASYNC_WEBSERVER)
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#define WEBSERVER_H

namespace RichHttp {
  namespace Generics {
    class ESP8266RequestHandler;

    namespace Configs {
      struct ESP8266Config : espressif_config<ESP8266WebServer, ESP8266RequestHandler> {
        using _fn_type = BuiltinFns::context_fn_type<ESP8266WebServer>::def::type;
        using _context_type = EspressifRequestContext<ESP8266WebServer>;

        static const _fn_type OtaHandlerFn;
        static const _fn_type OtaSuccessHandlerFn;
      };
      using EspressifBuiltin = ESP8266Config;
    };

    class ESP8266RequestHandler : public EspressifRequestHandler<Configs::ESP8266Config> {
      public:
        template <class... Args>
        ESP8266RequestHandler(Args... args) : EspressifRequestHandler<Configs::ESP8266Config>(args...) { }
    };
  };
};
#endif