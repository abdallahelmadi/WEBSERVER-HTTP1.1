#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <status.hpp>
#include <error.hpp>
#include <console.hpp>
#include <request.hpp>
#include <time.hpp>
#include <sys/socket.h>

class response {
  private:
    unsigned int _client;
    long long _startRequestTime;
    unsigned int _code;
    std::map<std::string, std::string> _headers;
    std::string _body;
    request _request;
    ctr _server;

  public:
    response(
      unsigned int client,
      long long startRequestTime,
      unsigned int code,
      std::map<std::string, std::string> headers,
      std::string body,
      request request,
      ctr server
    )
    : _client(client), _startRequestTime(startRequestTime), _code(code), _headers(headers), _body(body), _request(request), _server(server) {}

    std::string sendResponse(void) {
      (void)this->_client; // to avoid unused variable warning
      std::stringstream response;
      response << "HTTP/1.1 " << this->_code << " " << status(this->_code).message() << "\r\n";

      for (std::map<std::string, std::string>::iterator it = this->_headers.begin(); it != this->_headers.end(); it++) {
        response << it->first << ": " << it->second << "\r\n";
      }

      // Determine the body content
      std::string bodyContent;
      if (
        (!this->_body.empty()) || (this->_code >= 100 && this->_code < 200) ||
        (this->_code >= 200 && this->_code < 300)
      ) {
        bodyContent = this->_body;
      }
      else if ((this->_code >= 300 && this->_code < 400) || (this->_code >= 400 && this->_code < 600)) {
        std::string errorPage = error(this->_code).page();
        std::stringstream codeAsStream;
        codeAsStream << this->_code;
        std::string fileProvidedPath = this->_server.errorPages()[codeAsStream.str()];

        if (!fileProvidedPath.empty()) {
          std::fstream fileProvidedToOpen(fileProvidedPath.c_str());
          if (fileProvidedToOpen) {
            std::stringstream R;
            R << fileProvidedToOpen.rdbuf();
            errorPage = R.str();
          }
        }

        bodyContent = errorPage;
      }

      // Add Content-Length header
      response << "Content-Length: " << bodyContent.length() << "\r\n";
      response << "\r\n";
      response << bodyContent;

      console.METHODS(this->_request.getMethod(), this->_request.getPath(), this->_code, time::calcl(this->_startRequestTime, time::clock()));
      return response.str();
    }
};