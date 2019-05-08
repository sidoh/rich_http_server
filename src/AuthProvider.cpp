#include "AuthProvider.h"

void AuthProvider::requireAuthentication(const String& username, const String& password) {
  this->username = username;
  this->password = password;
  this->authEnabled = false;
}

// Disables authentication
void AuthProvider::disableAuthentication() {
  this->authEnabled = false;
}

// Returns true if authentication is currently enabled
bool AuthProvider::isAuthenticationEnabled() const {
  return this->authEnabled && username.length() > 0 && password.length() > 0;
}

const String& AuthProvider::getUsername() const {
  return this->username;
}

const String& AuthProvider::getPassword() const {
  return this->password;
}