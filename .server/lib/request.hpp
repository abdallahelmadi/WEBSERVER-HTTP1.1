#pragma once
#include <map>
#include <vector>
#include <iostream>

class request {
  private:
    std::string _method;
    std::string _path; // no query parameters handling
    std::string _http;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::size_t _badRequest;
  public:
    std::string const getMethod(void) const throw() { return this->_method; }
    std::string const getPath(void) const throw() { return this->_path; }
    std::string const getHTTP(void) const throw() { return this->_http; }
    std::map<std::string, std::string> const getHeaders(void) const throw() { return this->_headers; }
    std::string const getBody(void) const throw() { return this->_body; }

    request(std::string req): _method("UNDEFINED"), _path("UNDEFINED"), _http("UNDEFINED"), _badRequest(0) {

      // get method
      std::size_t pos = req.find(" ");
      if (pos == std::string::npos) {
        this->_badRequest = 400;
        return;
      }
      this->_method = req.substr(0, pos);
      if (this->_method != "GET" && this->_method != "POST" && this->_method != "DELETE") {
        if (this->_method == "PUT" || this->_method == "HEAD" || this->_method == "OPTIONS" || this->_method == "PATCH") {
          this->_badRequest = 405;
          return;
        }
        this->_badRequest = 501;
        return;
      }
      if (req[pos] == ' ')
        pos++;
      else {
        this->_badRequest = 400;
        return;
      }

      // get path
      std::size_t pos2 = req.find(" ", pos);
      if (pos2 == std::string::npos) {
        this->_badRequest = 400;
        return;
      }
      std::string pathTemp = req.substr(pos, pos2 - pos);
      if (pathTemp.empty() || pathTemp[0] != '/') {
        this->_badRequest = 400;
        return;
      }
      std::size_t posQuery = pathTemp.find("?");
      std::size_t posFragment = pathTemp.find("#");
      std::size_t endPath = pathTemp.length();
      if (posQuery != std::string::npos)
        endPath = std::min(endPath, posQuery);
      if (posFragment != std::string::npos)
        endPath = std::min(endPath, posFragment);
      this->_path = pathTemp.substr(0, endPath);
      if (req[pos2] == ' ')
        pos2++;
      else {
        this->_badRequest = 400;
        return;
      }

      // get http version
      std::size_t pos3 = req.find("\r\n", pos2);
      if (pos3 == std::string::npos) {
        this->_badRequest = 400;
        return;
      }
      this->_http = req.substr(pos2, pos3 - pos2);
      if (this->_http != "HTTP/1.1") {
        this->_badRequest = 505;
        return;
      }

      // parse headers & body
      std::size_t pos4 = pos3 + 2;
      while (true) {
        std::size_t posEndLine = req.find("\r\n", pos4);
        if (posEndLine == std::string::npos) {
          this->_badRequest = 400;
          return;
        }
        if (posEndLine == pos4) {
          // end of headers
          this->_body = req.substr(pos4 + 2);
          break;
        }
        std::size_t posColon = req.find(":", pos4);
        if (posColon == std::string::npos || posColon > posEndLine) {
          this->_badRequest = 400;
          return;
        }
        std::string headerKey = req.substr(pos4, posColon - pos4);
        if (headerKey.empty()) {
          this->_badRequest = 400;
          return;
        }
        for (std::size_t i = 0; i < headerKey.length(); i++) {
          if (!( (headerKey[i] >= 'A' && headerKey[i] <= 'Z') ||
                 (headerKey[i] >= 'a' && headerKey[i] <= 'z') ||
                 (headerKey[i] >= '0' && headerKey[i] <= '9') ||
                 headerKey[i] == '-' )) {
            this->_badRequest = 400;
            return;
          }
        }
        std::size_t posValueStart = posColon + 1;
        while (posValueStart < posEndLine && req[posValueStart] == ' ')
          posValueStart++;
        std::string headerValue = req.substr(posValueStart, posEndLine - posValueStart);
        this->_headers[headerKey] = headerValue;
        pos4 = posEndLine + 2;
      }
    }

    std::size_t getBadRequest(void) const throw() { return this->_badRequest; }
};