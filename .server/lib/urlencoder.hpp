#pragma once
#include <string>
#include <map>
#include "server.hpp"
#include <iostream>
#include <fstream>

std::string generate_uiid(size_t length);

class urlencoder {
  private:
    std::map<std::string, std::string> _fields;
  public:
    void parseBodyContent(std::string& content, ctr& currentServer);
};

void urlencoder::parseBodyContent(std::string& content, ctr& currentServer)
{
  std::size_t i = 0;
  while (i < content.length())
  {
    std::string key;
    std::string value;
    // split with &
    std::size_t pos = content.find('&', i);
    std::string pair;
    if (pos != std::string::npos)
    {
      pair = content.substr(i, pos - i);
      i = pos + 1;
    }
    else
    {
      pair = content.substr(i);
      i = content.length();
    }
    // split with =
    std::size_t pos_eq = pair.find('=');
    if (pos_eq != std::string::npos)
    {
      key = pair.substr(0, pos_eq);
      value = pair.substr(pos_eq + 1);
    }
    else
    {
      key = pair;
      value = "";
    }
    this->_fields[key] = value;
  }
  std::string filepath = currentServer.root() + generate_uiid(6) + "-" + "form_urlencoded.txt";
  std::ofstream outfile(filepath.c_str());
  for (std::map<std::string, std::string>::iterator it = this->_fields.begin(); it != this->_fields.end(); ++it) {
    outfile << "Field Name: " << it->first << ", Value: " << it->second << "\n";
  }
  outfile.close();
}