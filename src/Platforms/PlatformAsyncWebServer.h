#pragma once

#include "Generics.h"
#include <functional>

#if defined(RICH_HTTP_SERVER_ASYNC_WEBSERVER)
#include <ESPAsyncWebServer.h>

namespace RichHttp {
  namespace Generics {
    class AsyncRequestHandler;

    namespace Configs {
      using AsyncWebServer = Generics::HandlerConfig<
        AsyncWebServer,
        WebRequestMethodComposite,
        FunctionWrapper<void, AsyncWebServerRequest*, const UrlTokenBindings*)>,
        FunctionWrapper<
          void,
          AsyncWebServerRequest*,
          uint8_t* data,
          size_t len,
          size_t index,
          size_t total,
          const UrlTokenBindings*
        >,
        FunctionWrapper<
          void,
          AsyncWebServerRequest*,
          const String& filename,
          size_t index,
          uint8_t *data,
          size_t len,
          bool final,
          const UrlTokenBindings*
        >,
        AsyncRequestHandler
      >;
    };

    class AsyncRequestHandler : public ::RichHttp::Generics::RequestHandler<Configs::AsyncWebServer> {

    };
  };
};
#endif