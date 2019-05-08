#pragma once

#include <Arduino.h>

class AuthProvider {
  public:

    // Enables authentication using the provided username and password
    virtual void requireAuthentication(const String& username, const String& password);

    // Disables authentication
    virtual void disableAuthentication();

    // Returns true if authentication is currently enabled
    virtual bool isAuthenticationEnabled() const;

    virtual const String& getUsername() const;
    virtual const String& getPassword() const;

  private:
    bool authEnabled;
    String username;
    String password;
};