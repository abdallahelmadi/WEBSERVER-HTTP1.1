#pragma once
#include <iostream>
#include <vector>
#include <map>

class rt {
  private:
    std::string _path;
    std::string _source;
    bool _dictlist;
    std::string _redirect;
    std::string _cgiScript;
    std::string _cgiInterpreter;
    std::size_t _cgiTimeout;
    std::vector<std::string> _methods;
  public:
    rt(): _dictlist(false), _cgiTimeout(0) {}
    inline std::string& path(void) throw() { return this->_path; }
    inline bool& dictlist(void) throw() { return this->_dictlist; }
    inline std::string& redirect(void) throw() { return this->_redirect; }
    inline std::string& cgiScript(void) throw() { return this->_cgiScript; }
    inline std::string& cgiInterpreter(void) throw() { return this->_cgiInterpreter; }
    inline std::size_t& cgiTimeout(void) throw() { return this->_cgiTimeout; }
    inline std::string& source(void) throw() { return this->_source; }
    inline std::string& method(unsigned int index) throw() { return this->_methods[index]; }

    inline std::size_t length(void) const throw() { return this->_methods.size(); }

    std::string& create(void) throw() {
      std::string newN;
      this->_methods.push_back(newN);
      return this->_methods.back();
    }
    std::string& add(const std::string& str) throw() {
      this->_methods.push_back(str);
      return this->_methods.back();
    }
};

class ctr {
  private:
    std::size_t _port;
    std::string _name;
    std::string _version;
    std::map<std::string, std::string> _errorPages;
    std::string _log;
    std::size_t _bodylimit;
    std::size_t _timeout;
    std::string _uploaddir;
    std::string _index;
    std::string _root;
    std::vector<rt> _routes;
  public:
    inline std::size_t& port(void) throw() { return this->_port; }
    inline std::string& name(void) throw() { return this->_name; }
    inline std::string& version(void) throw() { return this->_version; }
    inline std::map<std::string, std::string>& errorPages(void) throw() { return this->_errorPages; }
    inline std::string& log(void) throw() { return this->_log; }
    inline std::size_t& bodylimit(void) throw() { return this->_bodylimit; }
    inline std::size_t& timeout(void) throw() { return this->_timeout; }
    inline std::string& uploaddir(void) throw() { return this->_uploaddir; }
    inline std::string& index(void) throw() { return this->_index; }
    inline std::string& root(void) throw() { return this->_root; }

    ctr(): _port(0), _bodylimit(0), _timeout(0) {}

    rt& route(unsigned int index) throw() { return this->_routes[index]; }
    inline std::size_t length(void) const throw() { return this->_routes.size(); }

    rt& create(void) throw() {
      rt newN;
      this->_routes.push_back(newN);
      return this->_routes.back();
    }
    rt& add(const rt& route) throw() {
      this->_routes.push_back(route);
      return this->_routes.back();
    }
};

class Server {
  private:
    std::vector<ctr> _servers;
  public:
    inline std::size_t length(void) const throw() { return this->_servers.size(); }
    ctr& operator[](unsigned int index) throw() { return this->_servers[index]; }

    ctr& create(void) throw() {
      ctr newN;
      this->_servers.push_back(newN);
      return this->_servers.back();
    }
    ctr& add(const ctr& server) throw() {
      this->_servers.push_back(server);
      return this->_servers.back();
    }
};

extern Server server;
/* global object declaration, see ./extern.cpp for actual definition */