#pragma once

#include <UrlTokenBindings.h>
#include <TokenIterator.h>

#include <functional>
#include <memory>

namespace RichHttp {
  namespace Generics {
    template <size_t...>
    struct ArgSequence;

    template <size_t I_0, size_t... I_ns>
    struct ArgSequenceGen : ArgSequenceGen<I_0 - 1, I_0 - 1, I_ns...>
    { };

    template <size_t... I_ns>
    struct ArgSequenceGen<0, I_ns...> {
      typedef ArgSequence<I_ns...> type;
    };

    template <class ReturnType, class... TArgs>
    struct FunctionWrapper {
      typedef std::function<ReturnType(TArgs...)> type;
    };

    template <
      class TServer,
      class THttpMethod,
      class TResponseHandlerReturn,
      class TRequestHandler,
      class TBodyRequestHandler,
      class TUploadRequestHandler,
      class TRequestHandlerClass
    >
    struct HandlerConfig {
      using ServerType = TServer;
      using HttpMethod = THttpMethod;
      using RequestHandlerFn = TRequestHandler;
      using BodyRequestHandlerFn = TBodyRequestHandler;
      using UploadRequestHandlerFn = TUploadRequestHandler;
      using RequestHandlerType = TRequestHandlerClass;
    };

    template<
      class TConfig,
      class TRequestHandler
    >
    struct ServerConfig {
      using ConfigType = TConfig;
      using RequestHandlerType = TRequestHandler;
    };

    template <class Config, class THandlerClass>
    class BaseRequestHandler : public THandlerClass {
      public:
        BaseRequestHandler(
          typename Config::HttpMethod method,
          const char* _path,
          typename Config::RequestHandlerFn::type handlerFn,
          typename Config::BodyRequestHandlerFn::type bodyFn = NULL,
          typename Config::UploadRequestHandlerFn::type uploadFn = NULL
        ) : method(method)
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
          if (this->method != HTTP_ANY && requestMethod != this->method) {
            return false;
          }

          return this->canHandlePath(path, length);
        }

      protected:
        typename Config::HttpMethod method;
        char* path;
        typename Config::RequestHandlerFn::type handlerFn;
        typename Config::BodyRequestHandlerFn::type bodyFn;
        typename Config::UploadRequestHandlerFn::type uploadFn;
        ::std::shared_ptr<TokenIterator> patternTokens;
    };
  };
};