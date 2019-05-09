#include <AuthProviders.h>

static const String NULL_RESPONSE = "";

const String& AuthProvider::getUsername() const {
  return NULL_RESPONSE;
}

const String& AuthProvider::getPassword() const {
  return NULL_RESPONSE;
}

SimpleAuthProvider::SimpleAuthProvider()
  : authEnabled(false)
{ }

void SimpleAuthProvider::requireAuthentication(const String& username, const String& password) {
  this->username = username;
  this->password = password;
  this->authEnabled = true;
}

void SimpleAuthProvider::disableAuthentication() {
  this->authEnabled = false;
}

bool SimpleAuthProvider::isAuthenticationEnabled() const {
  return this->authEnabled;
}

const String& SimpleAuthProvider::getUsername() const {
  return this->username;
}

const String& SimpleAuthProvider::getPassword() const {
  return this->password;
}