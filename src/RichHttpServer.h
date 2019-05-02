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

  void clearBuilders() {
    handlerBuilders.clear();
  }

  // Returns true if there's currently a client connected to the server.
  // This is only necessary for the builtin webservers
#if (defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)) && !defined(RICH_HTTP_ASYNC_WEBSERVER)
  bool isClientConnected() {
    return this->_currentClient && this->_currentClient.connected();
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
    if (! this->disableAuth) {
      fn = authedFnBuilder->buildAuthedFn(fn);
    }

    server.addHandler(new typename Config::RequestHandlerType(verb, path.c_str(), fn, nullptr, nullptr));
    return *this;
  }

  HandlerBuilder& on(const typename Config::HttpMethod verb, typename Config::RequestHandlerFn::type fn, typename Config::BodyRequestHandlerFn::type bodyFn) {
    if (! this->disableAuth) {
      fn = authedFnBuilder->buildAuthedFn(fn);
      bodyFn = authedFnBuilder->buildAuthedBodyFn(bodyFn);
    }

    server.addHandler(new typename Config::RequestHandlerType(path.c_str(), verb, fn, bodyFn, nullptr));
    return *this;
  }

  HandlerBuilder& onBody(const typename Config::HttpMethod verb, typename Config::BodyRequestHandlerFn::type bodyFn) {
    if (! this->disableAuth) {
      bodyFn = authedFnBuilder->buildAuthedBodyFn(bodyFn);
    }

    server.addHandler(new typename Config::RequestHandlerType(verb, path.c_str(), nullptr, bodyFn, nullptr));
    return *this;
  }

  HandlerBuilder& onUpload(typename Config::RequestHandlerFn::type fn, typename Config::UploadRequestHandlerFn::type uploadFn) {
    if (! this->disableAuth) {
      fn = authedFnBuilder->buildAuthedFn(fn);
      uploadFn = authedFnBuilder->buildAuthedUploadFn(uploadFn);
    }

    server.addHandler(new typename Config::RequestHandlerType(HTTP_POST, path.c_str(), fn, nullptr, uploadFn));
    return *this;
  }

  HandlerBuilder& onUpload(typename Config::UploadRequestHandlerFn::type uploadFn) {
    if (! this->disableAuth) {
      uploadFn = authedFnBuilder->buildAuthedUploadFn(uploadFn);
    }

    server.addHandler(new typename Config::RequestHandlerType(HTTP_POST, path.c_str(), nullptr, nullptr, uploadFn));
    return *this;
  }

private:
  bool disableAuth;
  const String path;
  RichHttpServer<Config>& server;
  typename Config::AuthedFnBuilderType* authedFnBuilder;
};