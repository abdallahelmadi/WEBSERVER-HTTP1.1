#include <server.hpp>
#include <iostream>

static void skipWhitespace(std::string& value) {
  std::size_t index = 0;
  while (value[index]) {
    if (
      value[index] == ' '  ||
      value[index] == '\n' ||
      value[index] == '\r' ||
      value[index] == '\t' ||
      value[index] == '\v' ||
      value[index] == '\f'
    )
      value.erase(index, 1);
    else
      index++;
  }
}

void json(std::string& configFileContent) {
  skipWhitespace(configFileContent);
  ;
}