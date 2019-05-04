#include "RichResponse.h"

namespace RichHttp {
  Response::Response(JsonDocument& json)
    : json(json)
    , responseCode(200)
  { }

  Response::~Response() { }

  void Response::sendRaw(int responseCode, const char* responseType, const char* body) {
    this->responseCode = responseCode;
    this->responseType = responseType;
    this->rawBody = body;
  }
}