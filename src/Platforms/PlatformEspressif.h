#pragma once

#include <UrlTokenBindings.h>
#include <ArduinoJson.h>
#include "Generics.h"
#include "../RichResponse.h"

#include <functional>

#if (defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)) && !defined(RICH_HTTP_ASYNC_WEBSERVER)

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif

namespace RichHttp {
  namespace Generics {
    template <class TServer>
    class EspressifRequestContext;

    namespace BuiltinFns {
      using handler_type = FunctionWrapper<void, const UrlTokenBindings*>;

      template <class TServer>
      struct context_fn_type {
        using def = FunctionWrapper<void, EspressifRequestContext<TServer>&>;
      };

      template <class TServer>
      struct upload_context_fn_type {
        using def = FunctionWrapper<void, EspressifRequestContext<TServer>&>;
      };
    };

    template <
      class TServerType,
      class THandler = BuiltinFns::handler_type,
      class TBodyHandler = BuiltinFns::handler_type,
      class TUploadHandler = BuiltinFns::handler_type,
      class TContextHandler = typename BuiltinFns::context_fn_type<TServerType>::def,
      class TUploadContextHandler = typename BuiltinFns::upload_context_fn_type<TServerType>::def
    >
    class EspressifHandlerFnWrapperBuilder;

    namespace Configs {
      template <
        class TServerType,
        class TRequestHandlerClass
      >
      struct espressif_config : Generics::HandlerConfig<
        TServerType,
        HTTPMethod,
        void*,
        void,
        typename BuiltinFns::handler_type,
        typename BuiltinFns::handler_type,
        typename BuiltinFns::handler_type,
        typename BuiltinFns::context_fn_type<TServerType>::def,
        typename BuiltinFns::upload_context_fn_type<TServerType>::def,
        TRequestHandlerClass,
        ::RichHttp::Generics::EspressifHandlerFnWrapperBuilder<TServerType>,
        EspressifRequestContext<TServerType>
      > { };
    };

    template <class TConfig>
    class EspressifRequestHandler : public ::RichHttp::Generics::BaseRequestHandler<TConfig, ::RequestHandler> {
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
            char requestUriCopy[uri.length() + 1];
            strcpy(requestUriCopy, uri.c_str());
            TokenIterator requestTokens(requestUriCopy, uri.length(), '/');

            UrlTokenBindings bindings(*(this->patternTokens), requestTokens);

            this->uploadFn(&bindings);
          }
        }
    };

    template <
      class TServerType,
      class THandler,
      class TBodyHandler,
      class TUploadHandler,
      class TContextHandler,
      class TUploadContextHandler
    >
    class EspressifHandlerFnWrapperBuilder
      : public HandlerFnWrapperBuilder<
          TServerType,
          THandler,
          TBodyHandler,
          TUploadHandler,
          TContextHandler,
          TUploadContextHandler
        >
    {
      public:
        template <class... Args>
        EspressifHandlerFnWrapperBuilder(Args... args)
          : HandlerFnWrapperBuilder<
              TServerType,
              THandler,
              TBodyHandler,
              TUploadHandler,
              TContextHandler,
              TUploadContextHandler
            >(args...) {}

        using fn_type = typename THandler::type;
        using body_fn_type = typename TBodyHandler::type;
        using upload_fn_type = typename TUploadHandler::type;
        using context_fn_type = typename TContextHandler::type;
        using upload_context_fn_type = typename TUploadContextHandler::type;

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
          return [this, fn, disableBody](const UrlTokenBindings* bindings) {
            DynamicJsonDocument responseDoc(RICH_HTTP_RESPONSE_BUFFER_SIZE);
            Response response(responseDoc);

            bool hasBody = !disableBody && this->server->hasArg("plain");

            EspressifRequestContext<TServerType> context(
              *this->server,
              response,
              *bindings,
              hasBody
            );

            fn(context);

            sendResponse(response);
          };
        }

        virtual upload_fn_type wrapUploadContextFn(upload_context_fn_type fn) override {
          return [this, fn](const UrlTokenBindings* bindings) {
            DynamicJsonDocument responseDoc(RICH_HTTP_RESPONSE_BUFFER_SIZE);
            Response response(responseDoc);

            EspressifRequestContext<TServerType> context(
              *this->server,
              response,
              *bindings,
              false
            );

            fn(context);

            sendResponse(response);
          };
        }

        void sendResponse(RichHttp::Response& response) {
          if (response.isSetBody()) {
            this->server->send(response.getCode(), response.getBodyType(), response.getBody());
          } else if (! response.json.isNull()) {
            this->server->setContentLength(measureJson(response.json));
            this->server->send_P(response.getCode(), ::RichHttp::CONTENT_TYPE_JSON, "");

            WiFiClient dest = this->server->client();
            serializeJson(response.json, dest);
          }
        }

        template <class RetType, class... Args>
        std::function<RetType(Args...)> buildAuthedHandler(std::function<RetType(Args...)> fn) {
          return [this, fn](Args... args) {
            if (this->authProvider->isAuthenticationEnabled()
              && !this->server->authenticate(this->authProvider->getUsername().c_str(), this->authProvider->getPassword().c_str()))
            {
              this->server->requestAuthentication();
            } else {
              return fn(args...);
            }
          };
        }
    };

    template <class TServer>
    class EspressifRequestContext : public RequestContext {
      public:
        template <class... Args>
        EspressifRequestContext(TServer& server, Response& response, Args... args)
          : RequestContext(response, args...)
          , server(server)
        { }

        virtual std::pair<const char*, size_t> loadBody() override {
          this->_body = this->server.arg("plain");
          return std::make_pair(this->_body.c_str(), this->_body.length());
        }

        TServer& server;

      private:
        // Have to keep a copy of the returned body.
        String _body;
    };
  };
};
#endif