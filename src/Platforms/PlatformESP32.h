#pragma once

#include "Generics.h"
#include "PlatformEspressif.h"

#include <functional>

#if defined(ARDUINO_ARCH_ESP32) && !defined(RICH_HTTP_ASYNC_WEBSERVER)
#include <WebServer.h>

namespace RichHttp {
  namespace Generics {
    class ESP32RequestHandler;

    namespace Configs {
      struct ESP32Config : espressif_config<WebServer, ESP32RequestHandler> {
        using _fn_type = BuiltinFns::context_fn_type<WebServer>::def::type;
        using _context_type = EspressifRequestContext<WebServer>;

        static const _fn_type OtaHandlerFn;
        static const _fn_type OtaSuccessHandlerFn;
      };
      using EspressifBuiltin = ESP32Config;
    };

    class ESP32RequestHandler : public EspressifRequestHandler<Configs::ESP32Config> {
      public:
        template <class... Args>
        ESP32RequestHandler(Args... args) : EspressifRequestHandler<Configs::ESP32Config>(args...) { }
    };
  };
};
#endif