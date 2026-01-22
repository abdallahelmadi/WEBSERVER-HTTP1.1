#pragma once
#include <string>
#include <vector>
#include <sys/stat.h>

extern std::vector<std::string> secureFiles;
extern std::vector<std::string> secureFolders;

class permission {
  public:
    static bool check(const std::string &path) {
      std::size_t pos = 0;
      for (size_t i = 0; i < secureFolders.size(); ++i) {
        if ((pos = path.find(secureFolders[i])) != std::string::npos) {
          pos += secureFolders[i].length();
          std::string sub = path.substr(0, pos);
          struct stat buffer;
          if (stat(sub.c_str(), &buffer) != 0)
            return true;
          if (S_ISREG(buffer.st_mode))
            continue;
          return true;
        }
      }
      for (size_t i = 0; i < secureFiles.size(); ++i) {
        if ((pos = path.find(secureFiles[i])) != std::string::npos) {
          pos += secureFiles[i].length();
          std::string sub = path.substr(0, pos);
          struct stat buffer;
          if (stat(sub.c_str(), &buffer) != 0)
            return true;
          if (S_ISREG(buffer.st_mode))
            return true;
        }
      }
      return false;
    }
};