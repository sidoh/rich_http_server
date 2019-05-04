#pragma once

#include <UrlTokenBindings.h>
#include <TokenIterator.h>
#include <ArduinoJson.h>

#include <functional>
#include <memory>

#ifndef RICH_HTTP_REQUEST_BUFFER_SIZE
#define RICH_HTTP_REQUEST_BUFFER_SIZE 1024
#endif

#ifndef RICH_HTTP_RESPONSE_BUFFER_SIZE
#define RICH_HTTP_RESPONSE_BUFFER_SIZE 1024
#endif

namespace RichHttp {
  static const char CONTENT_TYPE_JSON[] PROGMEM = "application/json";
  static const char CONTENT_TYPE_TEXT[] PROGMEM = "text/plain";

  namespace Generics {
    template <class ReturnType, class... TArgs>
    struct FunctionWrapper {
      using type = typename std::function<ReturnType(TArgs...)>;
    };

    template <
      class TServer,
      class THttpMethod,
      class TRequest,
      class TResponseHandlerReturn,
      class TRequestHandler,
      class TBodyRequestHandler,
      class TUploadRequestHandler,
      class TJsonRequestHandler,
      class TJsonBodyRequestHandler,
      class TRequestHandlerClass,
      class TAuthedFnBuilder
    >
    struct HandlerConfig {
      using ServerType = TServer;
      using HttpMethod = THttpMethod;
      using RequestType = TRequest;
      using ResponseHandlerReturnType = TResponseHandlerReturn;
      using RequestHandlerFn = TRequestHandler;
      using BodyRequestHandlerFn = TBodyRequestHandler;
      using UploadRequestHandlerFn = TUploadRequestHandler;
      using JsonRequestHandlerFn = TJsonRequestHandler;
      using JsonBodyRequestHandlerFn = TJsonBodyRequestHandler;
      using RequestHandlerType = TRequestHandlerClass;
      using AuthedFnBuilderType = TAuthedFnBuilder;
    };

    template<
      class TConfig,
      class TRequestHandler
    >
    struct ServerConfig {
      using ConfigType = TConfig;
      using RequestHandlerType = TRequestHandler;
    };

    template<
      class TServerType,
      class TMethodWrapper,
      class TBodyMethodWrapper,
      class TUploadMethodWrapper,
      class TJsonMethodWrapper,
      class TJsonBodyMethodWrapper
    >
    class AuthedFnBuilder {
      public:
        AuthedFnBuilder(TServerType* server, const AuthProvider* authProvider)
          : server(server)
          , authProvider(authProvider)
        {}

        virtual typename TMethodWrapper::type buildAuthedFn(typename TMethodWrapper::type) = 0;
        virtual typename TBodyMethodWrapper::type buildAuthedBodyFn(typename TBodyMethodWrapper::type) = 0;
        virtual typename TUploadMethodWrapper::type buildAuthedUploadFn(typename TUploadMethodWrapper::type) = 0;

        virtual typename TMethodWrapper::type wrapJsonFn(typename TJsonMethodWrapper::type) = 0;
        virtual typename TBodyMethodWrapper::type wrapJsonBodyFn(typename TJsonBodyMethodWrapper::type) = 0;

      protected:
        TServerType* server;
        const AuthProvider* authProvider;
    };

    template <class Config, class THandlerClass>
    class BaseRequestHandler : public THandlerClass {
      public:
        BaseRequestHandler(
          typename Config::HttpMethod anyMethod,
          typename Config::HttpMethod method,
          const char* _path,
          typename Config::RequestHandlerFn::type handlerFn,
          typename Config::BodyRequestHandlerFn::type bodyFn = NULL,
          typename Config::UploadRequestHandlerFn::type uploadFn = NULL
        ) : anyMethod(anyMethod)
          , method(method)
          , path(new char[strlen(_path) + 1])
          , handlerFn(handlerFn)
          , bodyFn(bodyFn)
          , uploadFn(uploadFn)
        {
          strcpy(this->path, _path);
          patternTokens = ::std::make_shared<TokenIterator>(this->path, strlen(this->path), '/');
        }

        virtual ~BaseRequestHandler() {
          delete[] path;
        }

        BaseRequestHandler& onUpload(typename Config::UploadRequestHandlerFn::type uploadFn) {
          this->uploadFn = uploadFn;
          return *this;
        }

        BaseRequestHandler& onBody(typename Config::BodyRequestHandlerFn::type uploadFn) {
          this->bodyFn = bodyFn;
          return *this;
        }

        bool canHandlePath(const char* requestPath, size_t length) {
          char requestUriCopy[length+1];
          strcpy(requestUriCopy, requestPath);
          TokenIterator requestTokens(requestUriCopy, length, '/');

          bool canHandle = true;

          patternTokens->reset();
          while (patternTokens->hasNext() && requestTokens.hasNext()) {
            const char* patternToken = patternTokens->nextToken();
            const char* requestToken = requestTokens.nextToken();

            if (patternToken[0] != ':' && strcmp(patternToken, requestToken) != 0) {
              canHandle = false;
              break;
            }

            if (patternTokens->hasNext() != requestTokens.hasNext()) {
              canHandle = false;
              break;
            }
          }

          return canHandle;
        }

        virtual bool _canHandle(typename Config::HttpMethod requestMethod, const char* path, size_t length) {
          if (this->method != this->anyMethod && requestMethod != this->method) {
            return false;
          }

          return this->canHandlePath(path, length);
        }

      protected:
        typename Config::HttpMethod anyMethod;
        typename Config::HttpMethod method;
        char* path;
        typename Config::RequestHandlerFn::type handlerFn;
        typename Config::BodyRequestHandlerFn::type bodyFn;
        typename Config::UploadRequestHandlerFn::type uploadFn;
        ::std::shared_ptr<TokenIterator> patternTokens;
    };
  };
};