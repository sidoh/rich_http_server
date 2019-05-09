#pragma once

#include <Arduino.h>

class AuthProvider {
public:
  // Returns true if authentication is currently enabled
  virtual bool isAuthenticationEnabled() const { return false; }

  virtual const String& getUsername() const;
  virtual const String& getPassword() const;
};

class SimpleAuthProvider : public AuthProvider {
public:
  SimpleAuthProvider();

  // Sets the provided username and password and enables authentication.
  void requireAuthentication(const String& username, const String& password);

  // Clears username and password and disables authentication
  void disableAuthentication();

  virtual bool isAuthenticationEnabled() const override;

  virtual const String& getUsername() const override;
  virtual const String& getPassword() const override;

private:
  String username;
  String password;
  bool authEnabled;
};