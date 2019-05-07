#include <Arduino.h>
#include <ArduinoJson.h>

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
    , fnWrapperBuilder(new typename Config::FnWrapperBuilderType(&server, &server))
  { }

  HandlerBuilder& setDisableAuthOverride() {
    this->disableAuth = true;
    return *this;
  }

  HandlerBuilder<Config>& handleOTA() {
    return on(HTTP_POST, Config::OtaSuccessHandlerFn, Config::OtaHandlerFn);
    // typename Config::BodyRequestHandlerFn::type wrappedFn = fnWrapperBuilder->wrapContextFn(Config::OtaHandlerFn, true);
    // typename Config::UploadRequestHandlerFn::type wrappedUploadFn = fnWrapperBuilder->wrapUploadContextFn(Config::OtaSuccessHandlerFn);

    // if (! this->disableAuth) {
    //   wrappedFn = fnWrapperBuilder->buildAuthedBodyFn(wrappedFn);
    //   wrappedUploadFn = fnWrapperBuilder->buildAuthedUploadFn(wrappedUploadFn);
    // }

    // server.addHandler(
    //   new typename Config::RequestHandlerType(
    //     HTTP_POST,
    //     path.c_str(),
    //     nullptr,
    //     wrappedFn,
    //     wrappedUploadFn
    //   )
    // );

    // return *this;
  }

  // Add handlers to the attached server.
  HandlerBuilder<Config>& onSimple(const typename Config::HttpMethod verb, typename Config::RequestHandlerFn::type fn) {
    if (! this->disableAuth) {
      fn = fnWrapperBuilder->buildAuthedFn(fn);
    }

    server.addHandler(new typename Config::RequestHandlerType(verb, path.c_str(), fn, nullptr, nullptr));
    return *this;
  }

  HandlerBuilder<Config>& on(
    const typename Config::HttpMethod verb,
    typename Config::ContextHandlerFn::type contextFn,
    typename Config::UploadContextHandlerFn::type uploadFn = nullptr
  ) {
    bool disableBody = uploadFn == nullptr || verb == HTTP_GET;
    typename Config::BodyRequestHandlerFn::type wrappedFn = fnWrapperBuilder->wrapContextFn(contextFn, disableBody);
    typename Config::UploadRequestHandlerFn::type wrappedUploadFn = nullptr;

    if (uploadFn != nullptr) {
      wrappedUploadFn = fnWrapperBuilder->wrapUploadContextFn(uploadFn);
    }

    if (! this->disableAuth) {
      wrappedFn = fnWrapperBuilder->buildAuthedBodyFn(wrappedFn);

      if (wrappedUploadFn) {
        wrappedUploadFn = fnWrapperBuilder->buildAuthedUploadFn(wrappedUploadFn);
      }
    }

    server.addHandler(new typename Config::RequestHandlerType(verb, path.c_str(), nullptr, wrappedFn, wrappedUploadFn));
    return *this;
  }

private:
  bool disableAuth;
  const String path;
  RichHttpServer<Config>& server;
  typename Config::FnWrapperBuilderType* fnWrapperBuilder;
};