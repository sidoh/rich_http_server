#pragma once

#include <ArduinoJson.h>

namespace RichHttp {
  class Response {
    public:
      Response(JsonDocument& json);
      ~Response();

    JsonDocument& json;

    void sendRaw(int responseCode, const char* responseType, const char* body);
    inline void setCode(int responseCode) { this->responseCode = responseCode; }

    inline bool isSetBody() const { return rawBody.length() > 0; }
    inline const String& getBody() const { return rawBody; }
    inline const String& getBodyType() const { return responseType; }
    inline int getCode() const { return responseCode; }

    private:
      int responseCode;
      String rawBody;
      String responseType;
  };
};