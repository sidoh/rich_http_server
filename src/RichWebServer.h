#include <Arduino.h>
#include <PathVariableHandler.h>

#if defined(ESP8266)
#include <ESP8266WebServer.h>

class RichWebServer : public ESP8266WebServer {
public:
  RichWebServer(int port) : ESP8266WebServer(port) { }
#elif defined(ESP32)
#include <WebServer.h>

class RichWebServer : public WebServer {
public:
  RichWebServer(int port) : WebServer(port) { }
#else
#error "Unsupported platform.  Only works with ESP8266 and ESP32"
#endif

  // Wrap the on(...) methods in a handler that requires authentication if enabled.
  void onAuthenticated(const String &uri, THandlerFunction handler);
  void onAuthenticated(const String &uri, HTTPMethod method, THandlerFunction fn);
  void onAuthenticated(const String &uri, HTTPMethod method, THandlerFunction fn, THandlerFunction ufn);

  // onPattern handlers allow for paths that have variables in them, e.g. /things/:thing_id.
  void onPattern(const String& pattern, const HTTPMethod method, PathVariableHandler::TPathVariableHandlerFn fn);
  void onPatternAuthenticated(const String& pattern, const HTTPMethod method, PathVariableHandler::TPathVariableHandlerFn handler);

  // Enables authentication using the provided username and password
  void requireAuthentication(const String& username, const String& password);

  // Disables authentication
  void disableAuthentication();

  // Returns true if authentication is currently enabled
  bool isAuthenticationEnabled() const;

private:
  bool authEnabled;
  String username;
  String password;

  bool validateAuthentication();
};