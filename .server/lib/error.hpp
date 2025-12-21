#pragma once
#include <string>
#include <sstream>
#include <status.hpp>

class error {
  private:
    std::string _page;
  public:
    error(int code) {
      std::stringstream page;
      page << "<html><head><title>" << status(code).message() << "</title></head><body>";
      page << "<h2>" << code << " " << status(code).message() << "</h2>";
      page << "</body></html>";
      this->_page = page.str();
    }
    std::string page() {
      return this->_page;
    }
};