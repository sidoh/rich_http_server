#pragma once

#include <UrlTokenBindings.h>
#include <TokenIterator.h>
#include <ArduinoJson.h>

#include <functional>
#include <memory>

#include "../RichResponse.h"
#include "../AuthProviders.h"

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
    /**
     * Contains a handler function's signature
     */
    template <class ReturnType, class... TArgs>
    struct FunctionWrapper {
      using type = typename std::function<ReturnType(TArgs...)>;
    };

    /**
     * Container for all of the platform-specific generics
     */
    template <
      class TServer,
      class THttpMethod,
      class TRequest,
      class TResponseHandlerReturn,
      class TRequestHandler,
      class TBodyRequestHandler,
      class TUploadRequestHandler,
      class TContextHandler,
      class TUploadContextHandler,
      class TRequestHandlerClass,
      class TFnWrapperBuilder,
      class TRequestContext
    >
    struct HandlerConfig {
      using ServerType = TServer;
      using HttpMethod = THttpMethod;
      using RequestType = TRequest;
      using ResponseHandlerReturnType = TResponseHandlerReturn;
      using RequestHandlerFn = TRequestHandler;
      using BodyRequestHandlerFn = TBodyRequestHandler;
      using UploadRequestHandlerFn = TUploadRequestHandler;
      using ContextHandlerFn = TContextHandler;
      using UploadContextHandlerFn = TUploadContextHandler;
      using RequestHandlerType = TRequestHandlerClass;
      using FnWrapperBuilderType = TFnWrapperBuilder;
      using RequestContextType = TRequestContext;
    };

    /**
     * Abstract class which serves the purpose of wrapping user-constructed handler functions.
     * There are two ways this is used:
     *
     *   1. Wrap a handler function so that authentication is processed.  When a request comes
     *      in and auth is enabled, the user's handler fn will be called iff auth validation
     *      succeeds.
     */
    template<
      class TServerType,
      class TMethodWrapper,
      class TBodyMethodWrapper,
      class TUploadMethodWrapper,
      class TContextHandler,
      class TUploadContextHandler
    >
    class HandlerFnWrapperBuilder {
      public:
        HandlerFnWrapperBuilder(TServerType* server, const AuthProvider* authProvider)
          : server(server)
          , authProvider(authProvider)
        {}

        virtual typename TMethodWrapper::type buildAuthedFn(typename TMethodWrapper::type) = 0;
        virtual typename TBodyMethodWrapper::type buildAuthedBodyFn(typename TBodyMethodWrapper::type) = 0;
        virtual typename TUploadMethodWrapper::type buildAuthedUploadFn(typename TUploadMethodWrapper::type) = 0;

        virtual typename TBodyMethodWrapper::type wrapContextFn(typename TContextHandler::type, bool disableBody) = 0;
        virtual typename TUploadMethodWrapper::type wrapUploadContextFn(typename TUploadContextHandler::type) = 0;

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
          patternTokens = std::make_shared<TokenIterator>(this->path, strlen(this->path), '/');
        }

        virtual ~BaseRequestHandler() {
          delete[] path;
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

    class RequestContext {
      public:
        RequestContext(
          Response& _response,
          const UrlTokenBindings& pathVariables,
          bool hasBody
        ) : response(_response)
          , pathVariables(pathVariables)
          , jsonBody(nullptr)
          , _hasBody(hasBody)
          , _bodyLoaded(false)
        { }

        Response& response;
        const UrlTokenBindings& pathVariables;

        virtual JsonDocument& getJsonBody() {
          if (jsonBody == nullptr) {
            jsonBody = parseJsonBody();
          }
          return *jsonBody;
        }

        virtual const char* getBody() {
          _loadBody();
          return *body;
        }

        virtual size_t getBodyLength() {
          _loadBody();
          return bodyLength;
        }

        virtual bool hasBody() {
          return this->_hasBody;
        }

      protected:
        virtual std::shared_ptr<JsonDocument> parseJsonBody() {
          std::shared_ptr<JsonDocument> jsonPtr = std::make_shared<DynamicJsonDocument>(RICH_HTTP_RESPONSE_BUFFER_SIZE);
          auto error = deserializeJson(*jsonPtr, this->getBody());

          if (error) {
            JsonObject err = response.json.createNestedObject("error");
            err["message"] = "Error parsing JSON";
            err["id"] = error.c_str();
            response.setCode(400);
          }

          return jsonPtr;
        }

        virtual std::pair<const char*, size_t> loadBody() = 0;

      private:
        std::shared_ptr<UrlTokenBindings> pathBindings;
        std::shared_ptr<JsonDocument> jsonBody;
        const char** body;
        size_t bodyLength;
        bool _hasBody;
        bool _bodyLoaded;

        void _loadBody() {
          if (! this->_bodyLoaded) {
            std::pair<const char*, size_t> body = loadBody();
            this->body = &body.first;
            this->bodyLength = body.second;
            this->_bodyLoaded = true;
          }
        }
    };
  };
};