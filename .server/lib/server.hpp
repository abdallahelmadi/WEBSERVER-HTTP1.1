#pragma once
#include <iostream>
#include <vector>

class rt {
  private:
    std::string _path;
    std::string _source;
    std::vector<std::string> _methods;
  public:
    inline std::string& path(void) throw() { return this->_path; }
    inline std::string& source(void) throw() { return this->_source; }
    inline std::vector<std::string>& methods(void) throw() { return this->_methods; }
};

class ctr {
  private:
    unsigned int _port;
    std::string _name;
    std::string _version;
    std::string _notfound;
    std::string _servererror;
    std::string _log;
    unsigned int _bodylimit;
    unsigned int _timeout;
    std::string _uploaddir;
    std::string _index;
    std::string _root;
    std::vector<rt> _routes;
  public:
    inline unsigned int& port(void) throw() { return this->_port; }
    inline std::string& name(void) throw() { return this->_name; }
    inline std::string& version(void) throw() { return this->_version; }
    inline std::string& notfound(void) throw() { return this->_notfound; }
    inline std::string& servererror(void) throw() { return this->_servererror; }
    inline std::string& log(void) throw() { return this->_log; }
    inline unsigned int& bodylimit(void) throw() { return this->_bodylimit; }
    inline unsigned int& timeout(void) throw() { return this->_timeout; }
    inline std::string& uploaddir(void) throw() { return this->_uploaddir; }
    inline std::string& index(void) throw() { return this->_index; }
    inline std::string& root(void) throw() { return this->_root; }

    rt& route(unsigned int index) throw() { return this->_routes[index]; }
};

class Server {
  private:
    std::vector<ctr> _servers;
  public:
    inline std::size_t length(void) const throw() { return this->_servers.size(); }
    ctr& operator[](unsigned int index) throw() { return this->_servers[index]; }
};

extern Server server;
/* global object declaration, see ./extern.cpp for actual definition */