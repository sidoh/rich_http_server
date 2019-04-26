#pragma once

#include <PathVariableHandler.h>

#if defined(PVH_ESP8266) || defined(PVH_ESP32)

class RichFunctionRequestHandler : public PathVariableHandler {
public:
    RichFunctionRequestHandler(
      PathVariableHandler::TPathVariableHandlerFn fn,
      TServerType::THandlerFunction ufn,
      const String& uri,
      HTTPMethod method
    )
    : PathVariableHandler(uri.c_str(), method, fn)
    , _ufn(ufn)
    { }

    bool canUpload(String requestUri) override  {
      return _ufn && canHandle(HTTP_POST, requestUri);
    }

    void upload(TServerType& server, String requestUri, HTTPUpload& upload) override {
      (void) server;
      (void) upload;
      if (canUpload(requestUri)) {
        _ufn();
      }
    }

protected:
    PathVariableHandler::TPathVariableHandlerFn _fn;
    TServerType::THandlerFunction _ufn;
    String _uri;
    HTTPMethod _method;
};
#endif