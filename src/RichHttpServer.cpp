#include <RichHttpServer.h>
#include <PathVariableHandler.h>

void RichHttpServer::onAuthenticated(const String &uri, THandlerFunction handler) {
  THandlerFunction authHandler = [this, handler]() {
    if (this->validateAuthentication()) {
      handler();
    }
  };

  TServerType::on(uri, authHandler);
}

void RichHttpServer::onAuthenticated(const String &uri, HTTPMethod method, THandlerFunction handler) {
  THandlerFunction authHandler = [this, handler]() {
    if (this->validateAuthentication()) {
      handler();
    }
  };

  TServerType::on(uri, method, authHandler);
}

void RichHttpServer::onAuthenticated(const String &uri, HTTPMethod method, THandlerFunction handler, THandlerFunction ufn) {
  THandlerFunction authHandler = [this, handler]() {
    if (this->validateAuthentication()) {
      handler();
    }
  };

  TServerType::on(uri, method, authHandler, ufn);
}

void RichHttpServer::onPattern(const String& pattern, const HTTPMethod method, PathVariableHandler::TPathVariableHandlerFn handler) {
  addHandler(new PathVariableHandler(pattern.c_str(), method, handler));
}

void RichHttpServer::onPatternAuthenticated(const String& pattern, const HTTPMethod method, PathVariableHandler::TPathVariableHandlerFn fn) {
  PathVariableHandler::TPathVariableHandlerFn authHandler = [this, fn](UrlTokenBindings* bindings) {
    if (this->validateAuthentication()) {
      fn(bindings);
    }
  };

  addHandler(new PathVariableHandler(pattern.c_str(), method, authHandler));
}

void RichHttpServer::requireAuthentication(const String& username, const String& password) {
  this->username = username;
  this->password = password;
  this->authEnabled = true;
}

void RichHttpServer::disableAuthentication() {
  this->authEnabled = false;
}

bool RichHttpServer::validateAuthentication() {
  if (this->authEnabled &&
    !authenticate(this->username.c_str(), this->password.c_str())) {
      requestAuthentication();
      return false;
    }
    return true;
}

bool RichHttpServer::isAuthenticationEnabled() const {
  return this->authEnabled;
}