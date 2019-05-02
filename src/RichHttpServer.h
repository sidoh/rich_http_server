#include <Arduino.h>

#include <vector>
#include <memory>

#include "AuthProvider.h"

#include "Platforms/Generics.h"
#include "Platforms/PlatformESP32.h"
#include "Platforms/PlatformESP8266.h"
#include "Platforms/PlatformAsyncWebServer.h"

template <class Config>
class HandlerBuilder;

template <class Config>
class RichHttpServer : public Config::ServerType, public AuthProvider {
public:
  RichHttpServer(int port) : Config::ServerType(port) { }
  ~RichHttpServer() { };

  HandlerBuilder<Config>& buildHandler(const String& path, bool disableAuth = false) {
    std::shared_ptr<HandlerBuilder<Config>> builder = std::make_shared<HandlerBuilder<Config>>(*this, path, disableAuth);
    handlerBuilders.push_back(builder);
    return *builder;
  }

  // Returns true if there's currently a client connected to the server.
#ifndef PVH_ASYNC_WEBSERVER
  bool isClientConnected() {
    return this->_currentClient && this->_currentClient.connected();
  }
#endif

  // Validates that client has provided auth for a particular request
#ifndef PVH_ASYNC_WEBSERVER
  bool validateAuthentication() {
    if (this->authEnabled && !this->authenticate(this->username.c_str(), this->password.c_str())) {
      this->requestAuthentication();
      return false;
    }
    return true;
  }
#else
  bool validateAuthentication(AsyncWebServerRequest* request) {
    return !isAuthenticationEnabled() || request->authenticate(username.c_str(), password.c_str());
  }
#endif

private:
  bool authEnabled;
  String username;
  String password;

  std::vector<std::shared_ptr<HandlerBuilder<Config>>> handlerBuilders;
};

template <class Config>
class HandlerBuilder {
public:
  HandlerBuilder(RichHttpServer<Config>& server, const String& path, const bool disableAuth = false)
    : disableAuth(disableAuth)
    , path(path)
    , server(server)
    , authedFnBuilder(new typename Config::AuthedFnBuilderType(&server, &server))
  { }

  HandlerBuilder& setDisableAuthOverride() {
    this->disableAuth = true;
    return *this;
  }

  // Add handlers to the attached server.
  HandlerBuilder<Config>& on(const typename Config::HttpMethod verb, typename Config::RequestHandlerFn::type fn) {
    server.addHandler(new typename Config::RequestHandlerType(verb, path.c_str(), authedFnBuilder->buildAuthedFn(fn), nullptr, nullptr));
    return *this;
  }

  HandlerBuilder& on(const typename Config::HttpMethod verb, typename Config::RequestHandlerFn::type fn, typename Config::BodyRequestHandlerFn::type bodyFn) {
    server.addHandler(new typename Config::RequestHandlerType(path.c_str(), verb, authedFnBuilder->buildAuthedFn(fn), authedFnBuilder->buildAuthedBodyFn(bodyFn), nullptr));
    return *this;
  }

  HandlerBuilder& onBody(const typename Config::HttpMethod verb, typename Config::BodyRequestHandlerFn::type bodyFn) {
    server.addHandler(new typename Config::RequestHandlerType(HTTP_POST, path.c_str(), nullptr, authedFnBuilder->buildAuthedBodyFn(bodyFn), nullptr));
    return *this;
  }

  HandlerBuilder& onUpload(typename Config::RequestHandlerFn::type fn, typename Config::UploadRequestHandlerFn::type uploadFn) {
    server.addHandler(new typename Config::RequestHandlerType(HTTP_POST, path.c_str(), authedFnBuilder->buildAuthedFn(fn), nullptr, authedFnBuilder->buildAuthedUploadFn(uploadFn)));
    return *this;
  }

  HandlerBuilder& onUpload(typename Config::UploadRequestHandlerFn::type uploadFn) {
    server.addHandler(new typename Config::RequestHandlerType(HTTP_POST, path.c_str(), nullptr, nullptr, authedFnBuilder->buildAuthedUploadFn(uploadFn)));
    return *this;
  }

private:
  bool disableAuth;
  const String path;
  RichHttpServer<Config>& server;
  typename Config::AuthedFnBuilderType* authedFnBuilder;

  // Wraps a given lambda in one that checks if auth is enabled, and validates auth if it is.
  // separate impls needed for the builtin HTTP server and AsyncWebServer.
#ifdef PVH_ASYNC_WEBSERVER
  template <class RetType, class ...Us>
  inline std::function<RetType(AsyncWebServerRequest*, Us... args)> buildAuthedHandler(std::function<RetType(AsyncWebServerRequest*, Us... args)> fn) {
    return [fn, this](AsyncWebServerRequest* r, Us... args) {
      if (disableAuth || server.validateAuthentication(r)) {
        return fn(r, args...);
      }
    };
  };
#else
  template <class RetType, class ...Us>
  inline std::function<RetType(Us... args)> buildAuthedHandler(std::function<RetType(Us... args)> fn) {
    return [fn, this](Us... args) {
      if (disableAuth || server.validateAuthentication()) {
        return fn(args...);
      }
    };
  };
#endif
};