#pragma once
#include <iostream>

class Console {
  public:
    inline void init(unsigned int port, std::string const network, std::string const name, std::string const version) const throw() {
      std::cout << "\n> " << name << "@" << version << " dev"
      << "\n> webserver dev -p " << port
      << "\n\n\033[1;38;2;43;14;68m   ▲ Webserver 1.0.0\033[0m\n"
      << "   - Local:   http://localhost:" << port
      << "\n   - Network: http://" << network << ":" << port
      << "\n\n \033[38;2;76;175;80m✓\033[0m Starting..." << std::endl;
    }

    inline void success(std::string const msg) const throw() { std::cout << " \033[38;2;76;175;80m✓\033[0m " << msg << std::endl; }
    inline void issue(std::string const msg) const throw() { std::cout << " \033[38;2;255;82;82m✗\033[0m " << msg << std::endl; }
    inline void info(std::string const msg) const throw() { std::cout << " \033[38;2;66;165;245m•\033[0m " << msg << std::endl; }
    inline void warning(std::string const msg) const throw() { std::cout << " \033[38;2;255;165;0m•\033[0m " << msg << std::endl; }
    inline void log(std::string const msg) const throw() { std::cout << " " << msg << std::endl; }

    inline void METHODS(std::string const method, std::string const path, unsigned int status, unsigned int time) const throw() {
      std::string clr;
      (status >= 200 && status < 300) ? clr = "\033[38;2;76;175;80m" :
      (status >= 300 && status < 400) ? clr = "\033[38;2;66;165;245m":
      (status >= 400 && status < 500) ? clr = "\033[38;2;255;165;0m" :
      (status >= 500 && status < 600) ? clr = "\033[38;2;255;82;82m" : clr = "\033[0m";
      std::cout << " " << method << " " << path << " " << clr << status
      << "\033[0m" << " in " << time << "ms" << std::endl;
    }
};

extern Console console;
/* global object declaration, see ./extern.cpp for actual definition */