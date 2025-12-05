#pragma once
#include <iostream>

class Console {
  public:
    void init(
      int port = 3000,
      std::string network = "127.0.0.1",
      std::string name = "www",
      std::string version = "0.1.0"
    ) const throw();
    void success(char const*) const throw();
    void issue(char const*) const throw();
    void info(char const*) const throw();
    void warning(char const*) const throw();
    void log(char const*) const throw();
    void GET(char const*, int, int) const throw();
    void POST(char const*, int, int) const throw();
    void DELETE(char const*, int, int) const throw();
};

extern Console console;
/* global object declaration, see ./console.cpp for actual definition */