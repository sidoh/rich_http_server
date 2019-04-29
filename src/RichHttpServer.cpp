#include <RichHttpServer.h>
#include <PathVariableHandler.h>
#include <RequestHandlers.h>

RichHttpServer::~RichHttpServer() {
  for (std::vector<HandlerBuilder*>::iterator itr = handlerBuilders.begin(); itr != handlerBuilders.end(); ++itr) {
    delete *itr;
  }
}

HandlerBuilder& RichHttpServer::buildHandler(const String& uri, bool disableAuth) {
  HandlerBuilder* builder = new HandlerBuilder(*this, uri, disableAuth);
  handlerBuilders.push_back(builder);
  return *builder;
}

void RichHttpServer::requireAuthentication(const String& username, const String& password) {
  this->username = username;
  this->password = password;
  this->authEnabled = true;
}

void RichHttpServer::disableAuthentication() {
  this->authEnabled = false;
}

#ifndef PVH_ASYNC_WEBSERVER
bool RichHttpServer::validateAuthentication() {
  if (this->authEnabled &&
    !authenticate(this->username.c_str(), this->password.c_str())) {
      requestAuthentication();
      return false;
    }
    return true;
}
#endif

bool RichHttpServer::isAuthenticationEnabled() const {
  return this->authEnabled;
}

HandlerBuilder::HandlerBuilder(RichHttpServer& server, const String& path, const bool disableAuth)
  : disableAuth(disableAuth)
  , path(path)
  , server(server)
{ }

HandlerBuilder& HandlerBuilder::setDisableAuthOverride() {
  this->disableAuth = true;
  return *this;
}