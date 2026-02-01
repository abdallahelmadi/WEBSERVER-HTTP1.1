#pragma once
#include <iostream>

class path {
  public:
    static bool check(std::string const& p1, std::string const& p2) {
      std::string path1 = normalize(p1);
      std::string path2 = normalize(p2);
      return (path1 == path2);
    }

  private:
    static std::string normalize(std::string const& p) {
      std::string result;
      bool lastWasSlash = false;
      
      for (std::size_t i = 0; i < p.length(); i++) {
        if (p[i] == '/') {
          if (!lastWasSlash) {
            result += '/';
            lastWasSlash = true;
          }
        } else {
          result += p[i];
          lastWasSlash = false;
        }
      }
      return result;
    }
};