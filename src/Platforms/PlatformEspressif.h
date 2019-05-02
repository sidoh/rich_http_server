#pragma once

#include <UrlTokenBindings.h>
#include "Generics.h"

#include <functional>

#if (defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)) && !defined(RICH_HTTP_ASYNC_WEBSERVER)

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif

namespace RichHttp {
  namespace Generics {
    template <
      class TServerType,
      class THandler = FunctionWrapper<void, const UrlTokenBindings*>,
      class TBodyHandler = FunctionWrapper<void, const UrlTokenBindings*>,
      class TUploadHandler = FunctionWrapper<void>
    >
    class EspressifAuthedFnBuilder;

    namespace Configs {
      template <
        class TServerType,
        class TRequestHandlerClass,
        class THandlerFn = FunctionWrapper<void, const UrlTokenBindings*>,
        class TUploadHandlerFn = FunctionWrapper<void>
      >
      struct espressif_config : Generics::HandlerConfig<
        TServerType,
        HTTPMethod,
        void*,
        void,
        THandlerFn,
        THandlerFn,
        TUploadHandlerFn,
        TRequestHandlerClass,
        ::RichHttp::Generics::EspressifAuthedFnBuilder<TServerType>
      > { };
    };

    template <class TConfig>
    class EspressifRequestHandler : public RichHttp::Generics::BaseRequestHandler<TConfig, ::RequestHandler> {
      public:

        template <class... Args>
        EspressifRequestHandler(Args... args) : RichHttp::Generics::BaseRequestHandler<TConfig, ::RequestHandler>(HTTP_ANY, args...)
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

          char requestUriCopy[uri.length() + 1];
          strcpy(requestUriCopy, uri.c_str());
          TokenIterator requestTokens(requestUriCopy, uri.length(), '/');

          UrlTokenBindings bindings(*(this->patternTokens), requestTokens);

          if (this->handlerFn) {
            this->handlerFn(&bindings);
          }

          if (this->bodyFn) {
            this->bodyFn(&bindings);
          }

          return true;
        }

        virtual void upload(typename TConfig::ServerType& server, String uri, HTTPUpload& upload) override {
          if (this->uploadFn) {
            this->uploadFn();
          }
        }
    };

    template <class TServerType, class THandler, class TBodyHandler, class TUploadHandler>
    class EspressifAuthedFnBuilder : public AuthedFnBuilder<TServerType, THandler, TBodyHandler, TUploadHandler> {
      public:
        template <class... Args>
        EspressifAuthedFnBuilder(Args... args) : AuthedFnBuilder<TServerType, THandler, TBodyHandler, TUploadHandler>(args...) {}

        using fn_type = typename THandler::type;
        using body_fn_type = typename TBodyHandler::type;
        using upload_fn_type = typename TUploadHandler::type;

        virtual fn_type buildAuthedFn(fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        virtual body_fn_type buildAuthedBodyFn(body_fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        virtual upload_fn_type buildAuthedUploadFn(upload_fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        template <class RetType, class... Args>
        std::function<RetType(Args...)> buildAuthedHandler(std::function<RetType(Args...)> fn) {
          return [this, fn](Args... args) {
            if (this->authProvider->isAuthenticationEnabled() && !this->server->authenticate(this->authProvider->getUsername().c_str(), this->authProvider->getPassword().c_str())) {
              this->server->requestAuthentication();
            } else {
              return fn(args...);
            }
          };
        }
    };
  };
};
#endif