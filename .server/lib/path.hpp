#pragma once
#include <iostream>

class path {
  public:
    static bool check(std::string const& p1, std::string const& p2) {
      std::string path1 = p1;
      std::string path2 = p2;
      std::size_t i = 0;
      while (path1[i]) {
        if (path1[i] == '/')
          path1.erase(i, 1);
        else
          i++;
      }
      i = 0;
      while (path2[i]) {
        if (path2[i] == '/')
          path2.erase(i, 1);
        else
          i++;
      }
      return (path1 == path2);
    }
};