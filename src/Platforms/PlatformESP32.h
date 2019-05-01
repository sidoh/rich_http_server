#pragma once

#include "Generics.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <WebServer.h>

namespace RichHttp {
  namespace Generics {
    class ESP32RequestHandler;

    namespace Configs {
      using ESP32Config = espressif_config<ESP32Config, ESP32RequestHandler>;
    };

    class ESP32RequestHandler : public EspressifRequestHandler<Configs::ESP32Config> { };
  };
};
#endif