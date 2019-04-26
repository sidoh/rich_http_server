#include <stddef.h>
#include <WString.h>

#pragma once

class AuthProvider {
public:
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
};