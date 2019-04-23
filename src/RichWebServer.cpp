#include <RichWebServer.h>
#include <PathVariableHandler.h>

void RichWebServer::onAuthenticated(const String &uri, THandlerFunction handler) {
  THandlerFunction authHandler = [this, handler]() {
    if (this->validateAuthentication()) {
      handler();
    }
  };

  TServerType::on(uri, authHandler);
}

void RichWebServer::onAuthenticated(const String &uri, HTTPMethod method, THandlerFunction handler) {
  THandlerFunction authHandler = [this, handler]() {
    if (this->validateAuthentication()) {
      handler();
    }
  };

  TServerType::on(uri, method, authHandler);
}

void RichWebServer::onAuthenticated(const String &uri, HTTPMethod method, THandlerFunction handler, THandlerFunction ufn) {
  THandlerFunction authHandler = [this, handler]() {
    if (this->validateAuthentication()) {
      handler();
    }
  };

  TServerType::on(uri, method, authHandler, ufn);
}

void RichWebServer::onPattern(const String& pattern, const HTTPMethod method, PathVariableHandler::TPathVariableHandlerFn handler) {
  addHandler(new PathVariableHandler(pattern.c_str(), method, handler));
}

void RichWebServer::onPatternAuthenticated(const String& pattern, const HTTPMethod method, PathVariableHandler::TPathVariableHandlerFn fn) {
  PathVariableHandler::TPathVariableHandlerFn authHandler = [this, fn](UrlTokenBindings* bindings) {
    if (this->validateAuthentication()) {
      fn(bindings);
    }
  };

  addHandler(new PathVariableHandler(pattern.c_str(), method, authHandler));
}

void RichWebServer::requireAuthentication(const String& username, const String& password) {
  this->username = username;
  this->password = password;
  this->authEnabled = true;
}

void RichWebServer::disableAuthentication() {
  this->authEnabled = false;
}

bool RichWebServer::validateAuthentication() {
  if (this->authEnabled &&
    !authenticate(this->username.c_str(), this->password.c_str())) {
      requestAuthentication();
      return false;
    }
    return true;
}

bool RichWebServer::isAuthenticationEnabled() const {
  return this->authEnabled;
}