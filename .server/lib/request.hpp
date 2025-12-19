#pragma once
#include <map>
#include <iostream>

// METHOD /path HTTP/version\r\n => METHOD+space+/path+space+HTTP/version+\r\n : Cannot be empty (GET /index.html HTTP/1.1\r\n)
// Header1: value1\r\n
// Header2: value2\r\n
// \r\n
// [optional body]

// GET / HTTP/1.1\r\n
// \r\n

class request {
  private:
    std::string _method;
    std::string _path; // no query parameters handling
    std::string _http;
    std::map<std::string, std::string> _headers;
    std::string _body;
  public:
    ;
};