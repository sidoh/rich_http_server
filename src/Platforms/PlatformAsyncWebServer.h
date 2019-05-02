#pragma once

#include "Generics.h"
#include <functional>

#if defined(_ESPAsyncWebServer_H_) || defined(RICH_HTTP_ASYNC_WEBSERVER)
#include <ESPAsyncWebServer.h>
#include <UrlTokenBindings.h>

#include <Arduino.h>
#include <stddef.h>

namespace RichHttp {
  namespace Generics {
    namespace AsyncFns {
      using handler_type = RichHttp::Generics::FunctionWrapper<void, AsyncWebServerRequest*, const UrlTokenBindings*>;
      using body_handler_type = RichHttp::Generics::FunctionWrapper<
        void,
        AsyncWebServerRequest*,
        const UrlTokenBindings*,
        uint8_t*,
        size_t,
        size_t,
        size_t
      >;
      using upload_handler_type = RichHttp::Generics::FunctionWrapper<
        void,
        AsyncWebServerRequest*,
        const UrlTokenBindings*,
        const String&,
        size_t,
        uint8_t*,
        size_t,
        bool
      >;
    };

    template <
      class TServerType = AsyncWebServer,
      class THandler = AsyncFns::handler_type,
      class TBodyHandler = AsyncFns::body_handler_type,
      class TUploadHandler = AsyncFns::upload_handler_type
    >
    class AsyncAuthedFnBuilder : public AuthedFnBuilder<TServerType, THandler, TBodyHandler, TUploadHandler> {
      public:

        template <class... Args>
        AsyncAuthedFnBuilder(Args... args)
          : AuthedFnBuilder<TServerType, THandler, TBodyHandler, TUploadHandler>
        (args...) {}

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
        std::function<RetType(AsyncWebServerRequest*, Args...)> buildAuthedHandler(
          std::function<RetType(AsyncWebServerRequest*, Args...)> fn
        ) {
          return [this, fn](AsyncWebServerRequest* request, Args... args) {
            if (!this->authProvider->isAuthenticationEnabled()
              || request->authenticate(this->authProvider->getUsername().c_str(), this->authProvider->getPassword().c_str())) {
              return fn(request, args...);
            }
          };
        }
    };

    class AsyncRequestHandler;

    namespace Configs {
      using AsyncWebServer = Generics::HandlerConfig<
        ::AsyncWebServer,
        WebRequestMethodComposite,
        AsyncWebServerRequest*,
        void,
        typename AsyncFns::handler_type,
        typename AsyncFns::body_handler_type,
        typename AsyncFns::upload_handler_type,
        ::RichHttp::Generics::AsyncRequestHandler,
        ::RichHttp::Generics::AsyncAuthedFnBuilder<>
      >;
    };

    class AsyncRequestHandler : public ::RichHttp::Generics::BaseRequestHandler<Configs::AsyncWebServer, ::AsyncWebHandler> {
      public:

        template <class... Args>
        AsyncRequestHandler(Args... args)
          : ::RichHttp::Generics::BaseRequestHandler<Configs::AsyncWebServer, ::AsyncWebHandler>
        (HTTP_ANY, args...) { }

        virtual bool canHandle(AsyncWebServerRequest* request) override {
          return this->_canHandle(request->method(), request->url().c_str(), request->url().length());
        }

        virtual void handleRequest(AsyncWebServerRequest* request) override {
          forwardToHandler<>(this->handlerFn, request);
        }

        virtual void handleUpload(
          AsyncWebServerRequest *request,
          const String& filename,
          size_t index,
          uint8_t *data,
          size_t len,
          bool isFinal
        ) override {
          forwardToHandler<const String&, size_t, uint8_t*, size_t, bool>(this->uploadFn, request, filename, index, data, len, isFinal);
        }

        virtual void handleBody(
          AsyncWebServerRequest *request,
          uint8_t *data,
          size_t len,
          size_t index,
          size_t total
        ) override {
          forwardToHandler(this->bodyFn, request, data, len, index, total);
        }

        template <class... OtherArgs>
        void forwardToHandler(
          std::function<void(AsyncWebServerRequest*, const UrlTokenBindings*, OtherArgs...)> fn,
          AsyncWebServerRequest* request,
          OtherArgs... args
        ) {
          if (fn) {
            char requestUriCopy[request->url().length() + 1];
            strcpy(requestUriCopy, request->url().c_str());
            TokenIterator requestTokens(requestUriCopy, request->url().length(), '/');

            UrlTokenBindings bindings(*(this->patternTokens), requestTokens);

            fn(request, &bindings, args...);
          }
        }
    };
  };
};
#endif