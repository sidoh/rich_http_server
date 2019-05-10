#pragma once

#include "Generics.h"
#include <functional>

#if defined(_ESPAsyncWebServer_H_) || defined(RICH_HTTP_ASYNC_WEBSERVER)
#include <ESPAsyncWebServer.h>
#include <UrlTokenBindings.h>
#include <ArduinoJson.h>

#include <Arduino.h>
#include <stddef.h>

namespace RichHttp {
  namespace Generics {
    class AsyncRequestContext;

    struct BodyArgs {
      uint8_t* data;
      size_t length;
      size_t index;
      size_t total;

      bool hasBody() const {
        return data != nullptr && length > 0;
      }
    };

    struct UploadArgs {
      const String& filename;
      size_t index;
      uint8_t *data;
      size_t length;
      bool isFinal;

      bool hasUpload() const {
        return data != nullptr && length > 0;
      }
    };

    // Use when creating a request context with no upload
    static const String NULL_FILENAME;

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
      using context_fn_type = FunctionWrapper<void, AsyncRequestContext&>;
    };

    template <
      class TServerType = AsyncWebServer,
      class THandler = AsyncFns::handler_type,
      class TBodyHandler = AsyncFns::body_handler_type,
      class TUploadHandler = AsyncFns::upload_handler_type,
      class TContextHandler = AsyncFns::context_fn_type,
      class TUploadContextHandler = AsyncFns::context_fn_type
    >
    class AsyncHandlerFnWrapperBuilder
      : public HandlerFnWrapperBuilder<
        TServerType,
        THandler,
        TBodyHandler,
        TUploadHandler,
        TContextHandler,
        TUploadContextHandler
      > {
      public:

        template <class... Args>
        AsyncHandlerFnWrapperBuilder(Args... args)
          : HandlerFnWrapperBuilder<TServerType, THandler, TBodyHandler, TUploadHandler, TContextHandler, TUploadContextHandler>
        (args...) {}

        using fn_type = typename THandler::type;
        using body_fn_type = typename TBodyHandler::type;
        using upload_fn_type = typename TUploadHandler::type;
        using context_fn_type = typename TContextHandler::type;

        virtual fn_type buildAuthedFn(fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        virtual body_fn_type buildAuthedBodyFn(body_fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        virtual upload_fn_type buildAuthedUploadFn(upload_fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        virtual body_fn_type wrapContextFn(context_fn_type fn, bool disableBody) override {
          return [this, fn, disableBody](
            AsyncWebServerRequest* request,
            const UrlTokenBindings* bindings,
            uint8_t* data,
            size_t length,
            size_t index,
            size_t total
          ) {
            DynamicJsonDocument responseDoc(RICH_HTTP_RESPONSE_BUFFER_SIZE);
            Response response(responseDoc);

            bool hasBody = !disableBody && length > 0;

            AsyncRequestContext context(
              BodyArgs{ .data = data, .length = length, .index = index, .total = total },
              UploadArgs{
                .filename = NULL_FILENAME,
                .index = 0,
                .data = nullptr,
                .length = 0,
                .isFinal = true
              },
              request,
              response,
              *bindings,
              hasBody
            );

            fn(context);

            sendResponse(request, response);
          };
        }

        virtual upload_fn_type wrapUploadContextFn(context_fn_type fn) override {
          return [this, fn](
            AsyncWebServerRequest *request,
            const UrlTokenBindings* bindings,
            const String& filename,
            size_t index,
            uint8_t *data,
            size_t length,
            bool isFinal
          ) {
            DynamicJsonDocument responseDoc(RICH_HTTP_RESPONSE_BUFFER_SIZE);
            Response response(responseDoc);

            AsyncRequestContext context(
              BodyArgs{ .data = nullptr, .length = 0, .index = 0, .total = 0 },
              UploadArgs{
                .filename = filename,
                .index = index,
                .data = data,
                .length = length,
                .isFinal = isFinal
              },
              request,
              response,
              *bindings,
              false
            );

            fn(context);

            sendResponse(request, response);
          };
        }

        void sendResponse(AsyncWebServerRequest* request, RichHttp::Response& response) {
          if (response.isSetBody()) {
            request->send(response.getCode(), response.getBodyType(), response.getBody());
          } else if (! response.json.isNull()) {
            String body;
            serializeJson(response.json, body);
            request->send(response.getCode(), ::RichHttp::CONTENT_TYPE_JSON, body);
          }
        }

        template <class RetType, class... Args>
        std::function<RetType(AsyncWebServerRequest*, Args...)> buildAuthedHandler(
          std::function<RetType(AsyncWebServerRequest*, Args...)> fn
        ) {
          return [this, fn](AsyncWebServerRequest* request, Args... args) {
            if (!this->authProvider->isAuthenticationEnabled()
              || request->authenticate(this->authProvider->getUsername().c_str(), this->authProvider->getPassword().c_str())) {
              return fn(request, args...);
            } else {
              return request->requestAuthentication();
            }
          };
        }
    };

    class AsyncRequestHandler;

    namespace Configs {
      struct AsyncWebServer : Generics::HandlerConfig<
        ::AsyncWebServer,
        WebRequestMethodComposite,
        AsyncWebServerRequest*,
        void,
        typename AsyncFns::handler_type,
        typename AsyncFns::body_handler_type,
        typename AsyncFns::upload_handler_type,
        typename AsyncFns::context_fn_type,
        typename AsyncFns::context_fn_type,
        ::RichHttp::Generics::AsyncRequestHandler,
        ::RichHttp::Generics::AsyncHandlerFnWrapperBuilder<>,
        AsyncRequestContext
      >
      {
        using _fn_type = AsyncFns::context_fn_type::type;
        using _context_type = AsyncRequestContext;

        static const _fn_type OtaHandlerFn;
        static const _fn_type OtaSuccessHandlerFn;
      };
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

        virtual bool isRequestHandlerTrivial() override { return false; }

        virtual void handleRequest(AsyncWebServerRequest* request) override {
          if (this->handlerFn) {
            forwardToHandler<>(this->handlerFn, request);
          // Will already have been called if there's non-zero content length
          } else if (this->uploadFn || request->contentLength() == 0) {
            forwardToHandler<uint8_t*, size_t, size_t, size_t>(this->bodyFn, request, nullptr, 0, 0, 0);
          }
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

    class AsyncRequestContext : public RequestContext {
      public:
        template <class... Args>
        AsyncRequestContext(
          BodyArgs bodyArgs,
          UploadArgs uploadArgs,
          AsyncWebServerRequest* request,
          Response& response,
          Args... args
        ) : RequestContext(response, args...)
          , body(bodyArgs)
          , upload(uploadArgs)
          , rawRequest(request)
        { }

        virtual std::pair<const char*, size_t> loadBody() override {
          return std::make_pair(reinterpret_cast<const char*>(body.data), body.length);
        }

        const BodyArgs body;
        const UploadArgs upload;
        AsyncWebServerRequest* rawRequest;

      private:
        String loadedBody;
    };
  };
};
#endif