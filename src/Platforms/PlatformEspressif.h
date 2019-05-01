#pragma once

#include <UrlTokenBindings.h>
#include "Generics.h"

#include <functional>

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
namespace RichHttp {
  namespace Generics {
    namespace Configs {
      template <class TServerType, class TRequestHandlerClass>
      struct espressif_config : Generics::HandlerConfig<
        TServerType,
        HTTPMethod,
        void,
        FunctionWrapper<void, const UrlTokenBindings*>,
        FunctionWrapper<void>,
        FunctionWrapper<void>,
        TRequestHandlerClass
      > { };
    };

    template <class TConfig>
    class EspressifRequestHandler : public RichHttp::Generics::BaseRequestHandler<TConfig, ::RequestHandler> {
      public:

        template <class... Args>
        EspressifRequestHandler(Args... args) : RichHttp::Generics::BaseRequestHandler<TConfig, ::RequestHandler>(args...)
        { }

        virtual bool canHandle(typename TConfig::HttpMethod method, String uri) override {
          return this->_canHandle(method, uri.c_str(), uri.length());
        }

        virtual bool canUpload(String uri) override {
          return this->uploadFn && this->_canHandle(HTTP_POST, uri.c_str(), uri.length());
        }

        virtual bool handle(typename TConfig::ServerType& server, typename TConfig::HttpMethod method, String uri) override {
          if (! this->canHandle(method, uri)) {
            return false;
          }

          char requestUriCopy[uri.length()];
          strcpy(requestUriCopy, uri.c_str());
          TokenIterator requestTokens(requestUriCopy, uri.length(), '/');

          UrlTokenBindings bindings(*(this->patternTokens), requestTokens);
          this->handlerFn(&bindings);

          return true;
        }

        virtual void upload(typename TConfig::ServerType& server, String uri, HTTPUpload& upload) override {
          if (this->uploadFn) {
            this->uploadFn();
          }
        }
    };
  };
};
#endif