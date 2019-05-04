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
    namespace BuiltinFns {
      using base_type = FunctionWrapper<void>;
      using handler_type = FunctionWrapper<void, const UrlTokenBindings*>;
      using json_type = FunctionWrapper<void, const UrlTokenBindings*, RichHttp::Response&>;
      using json_body_type = FunctionWrapper<void, const UrlTokenBindings*, JsonDocument&, RichHttp::Response&>;
    };

    template <
      class TServerType,
      class THandler = BuiltinFns::handler_type,
      class TBodyHandler = BuiltinFns::handler_type,
      class TUploadHandler = BuiltinFns::base_type,
      class TJsonHandler = BuiltinFns::json_type,
      class TJsonBodyHandler = BuiltinFns::json_body_type
    >
    class EspressifAuthedFnBuilder;

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
        typename BuiltinFns::base_type,
        typename BuiltinFns::json_type,
        typename BuiltinFns::json_body_type,
        TRequestHandlerClass,
        ::RichHttp::Generics::EspressifAuthedFnBuilder<TServerType>
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
            this->uploadFn();
          }
        }
    };

    template <
      class TServerType,
      class THandler,
      class TBodyHandler,
      class TUploadHandler,
      class TJsonHandler,
      class TJsonBodyHandler
    >
    class EspressifAuthedFnBuilder
      : public AuthedFnBuilder<TServerType, THandler, TBodyHandler, TUploadHandler, TJsonHandler, TJsonBodyHandler>
    {
      public:
        template <class... Args>
        EspressifAuthedFnBuilder(Args... args)
          : AuthedFnBuilder<
              TServerType,
              THandler,
              TBodyHandler,
              TUploadHandler,
              TJsonHandler,
              TJsonBodyHandler
            >(args...) {}

        using fn_type = typename THandler::type;
        using body_fn_type = typename TBodyHandler::type;
        using upload_fn_type = typename TUploadHandler::type;
        using json_fn_type = typename TJsonHandler::type;
        using json_body_fn_type = typename TJsonBodyHandler::type;

        virtual fn_type buildAuthedFn(fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        virtual body_fn_type buildAuthedBodyFn(body_fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        virtual upload_fn_type buildAuthedUploadFn(upload_fn_type fn) override {
          return buildAuthedHandler(fn);
        }

        virtual fn_type wrapJsonFn(json_fn_type fn) override {
          return [this, fn](const UrlTokenBindings* bindings) {
            DynamicJsonDocument responseDoc(RICH_HTTP_RESPONSE_BUFFER_SIZE);
            RichHttp::Response response(responseDoc);

            fn(bindings, response);

            sendResponse(response);
          };
        }

        virtual body_fn_type wrapJsonBodyFn(json_body_fn_type fn) override {
          return [this, fn](const UrlTokenBindings* bindings) {
            DynamicJsonDocument requestDoc(RICH_HTTP_REQUEST_BUFFER_SIZE);

            auto error = deserializeJson(requestDoc, this->server->arg("plain"));

            if (error) {
              this->server->setContentLength(strlen(error.c_str()));
              this->server->send_P(400, ::RichHttp::CONTENT_TYPE_TEXT, PSTR(""));
              this->server->sendContent(error.c_str());
              Serial.printf_P(PSTR("Error parsing client-sent JSON: %s\n"), error.c_str());
              return;
            }

            DynamicJsonDocument responseDoc(RICH_HTTP_RESPONSE_BUFFER_SIZE);
            RichHttp::Response response(responseDoc);

            fn(bindings, requestDoc, response);

            sendResponse(response);
          };
        }

        void sendResponse(RichHttp::Response& response) {
          if (response.isSetBody()) {
            this->server->send(response.getCode(), response.getBodyType(), response.getBody());
          } else {
            this->server->setContentLength(measureJson(response.json));
            this->server->send_P(response.getCode(), ::RichHttp::CONTENT_TYPE_JSON, PSTR(""));

            WiFiClient dest = this->server->client();
            serializeJson(response.json, dest);
          }
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