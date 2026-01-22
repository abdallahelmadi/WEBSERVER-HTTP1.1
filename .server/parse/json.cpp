#include <server.hpp>
#include <cstdlib>

static void fillDefault(void) {
  std::size_t defaultPort = 3000;
  for (std::size_t i = 0; i < server.length(); i++) {
    if (!server[i].port()) {
      server[i].port() = defaultPort;
      defaultPort++;
    }
    if (server[i].name().empty())
      throw std::runtime_error("web application name is required");
    else
      server[i].name() = server[i].name();
    if (server[i].version().empty())
      server[i].version() = "0.1.0";
    if (server[i].root().empty())
      server[i].root() = "app/" + server[i].name() + "/";
    else
      server[i].root() = "app/" + server[i].name() + "/" + server[i].root() + "/";
    if (server[i].index().empty())
      server[i].index() = "index.html";
    else {
      if (server[i].index()[0] == '.' || server[i].index()[0] == '/')
        throw std::runtime_error("index file must be relative");
    }
    for (std::map<std::string, std::string>::iterator it = server[i].errorPages().begin();
         it != server[i].errorPages().end(); ++it) {
      if (it->second.empty())
        throw std::runtime_error("error page path cannot be empty for code: " + it->first);
      it->second = server[i].root() + it->second;
    }
    if (server[i].log().empty())
      server[i].log() = ".server/.log/" + server[i].name() + "/" + server[i].name() + ".log";
    else
      server[i].log() = server[i].root() + server[i].log();
    if (server[i].uploaddir().empty())
      server[i].uploaddir() = server[i].root() + "/uploads";
    else
      server[i].uploaddir() = server[i].root() + server[i].uploaddir();
    if (!server[i].bodylimit())
      server[i].bodylimit() = 1048576;
    if (!server[i].timeout())
      server[i].timeout() = 30000;
    if (server[i].length() != 0) {
      std::vector<std::string> tempPaths;
      for (std::size_t j = 0; j < server[i].length(); j++) {
        if (server[i].route(j).source().empty())
          server[i].route(j).source() = server[i].root() + server[i].route(j).path();
        else
          server[i].route(j).source() = server[i].root() + server[i].route(j).source();
        if (server[i].route(j).path().empty())
          throw std::runtime_error("route path is required");
        else if (server[i].route(j).path().find('?') != std::string::npos || server[i].route(j).path().find('#') != std::string::npos)
          throw std::runtime_error("route path cannot contain query string (queries are ignored, don't handled yet)");
        tempPaths.push_back(server[i].route(j).path());
        if (server[i].route(j).path()[0] != '/')
          throw std::runtime_error("route path must start with '/'");
        for (std::size_t xx = 0; xx < server[i].route(j).path().length(); xx++) {
          if (server[i].route(j).path()[xx] == '/' && server[i].route(j).path()[xx + 1] == '/')
            throw std::runtime_error("route path cannot contain double slashes");
          if (server[i].route(j).path()[xx] == '\\')
            throw std::runtime_error("route path cannot contain backslashes");
          if (
            server[i].route(j).path()[xx] == ' '  ||
            server[i].route(j).path()[xx] == '\n' ||
            server[i].route(j).path()[xx] == '\r' ||
            server[i].route(j).path()[xx] == '\t' ||
            server[i].route(j).path()[xx] == '\v' ||
            server[i].route(j).path()[xx] == '\f')
            throw std::runtime_error("route path cannot contain whitespace characters");
        }
      }
      for (std::size_t m = 0; m < tempPaths.size(); m++) {
        for (std::size_t n = m + 1; n < tempPaths.size(); n++) {
          if (tempPaths[m] == tempPaths[n])
            throw std::runtime_error("duplicate method in route: " + tempPaths[m]);
        }
      }
      tempPaths.clear();
      for (std::size_t j = 0; j < server[i].length(); j++) {
        rt& route = server[i].route(j);
        if (route.length() == 0) {
          route.add("GET");
          route.add("POST");
        }
        std::vector<std::string> tempMethods;
        for (std::size_t k = 0; k < route.length(); k++) {
          tempMethods.push_back(route.method(k));
        }
        for (std::size_t m = 0; m < tempMethods.size(); m++) {
          for (std::size_t n = m + 1; n < tempMethods.size(); n++) {
            if (tempMethods[m] == tempMethods[n])
              throw std::runtime_error("duplicate method in route: " + tempMethods[m]);
          }
        }
        tempMethods.clear();
      }
    }
    for (std::size_t j = 0; j < server[i].length(); j++) {
      rt& route = server[i].route(j);
      bool hasCgiScript = !route.cgiScript().empty();
      bool hasCgiInterpreter = !route.cgiInterpreter().empty();
      bool hasCgiTimeout = route.cgiTimeout() != 0;
      bool hasCgi = hasCgiScript || hasCgiInterpreter || hasCgiTimeout;
      if (hasCgi) {
        if (!route.redirect().empty())
          throw std::runtime_error("CGI route cannot have redirect field");
        if (route.cgiScript().empty())
          throw std::runtime_error("CGI route missing cgi_script");
        if (route.cgiInterpreter().empty())
          throw std::runtime_error("CGI route missing cgi_interpreter");
        if (route.cgiTimeout() == 0)
          route.cgiTimeout() = 1000;
      }
      if (!route.redirect().empty()
          && hasCgi)
        throw std::runtime_error("redirect route cannot have cgi settings");
      if (!route.cgiScript().empty())
        route.cgiScript() = server[i].root() + route.cgiScript();
    }
  }
  std::vector<std::string> temp;
  for (std::size_t i = 0; i < server.length(); i++) {
    temp.push_back(server[i].name());
  }
  for (std::size_t i = 0; i < temp.size(); i++) {
    for (std::size_t j = i + 1; j < temp.size(); j++) {
      if (temp[i] == temp[j])
        throw std::runtime_error("duplicate web application: " + temp[i]);
    }
  }
  temp.clear();
}

std::vector<std::string> serverKeys;
std::vector<std::string> routeKeys;

static void skipWhitespace(std::string &value) {
  std::size_t index = 0;
  bool insideQuotes = false;

  while (value[index]) {
    bool whiteSpaces = value[index] == ' ' || value[index] == '\n' || value[index] == '\r' ||
      value[index] == '\t' || value[index] == '\v' || value[index] == '\f';

    if (value[index] == '\"')
      insideQuotes = !insideQuotes;

    if (whiteSpaces && !insideQuotes)
      value.erase(index, 1);
    else
      index++;
  }
}

static void duplicateKey(std::vector<std::string> &temp, const std::string &key) {
  for (std::size_t i = 0; i < temp.size(); i++) {
    if (temp[i] == key)
      throw std::runtime_error("duplicate key: " + key);
  }
  temp.push_back(key);
}

static int isDigit(std::string const &value) {
  std::size_t z = 0;
  if (value[z] == '-')
    z++;
  for (std::size_t i = z; i < value.length(); i++) {
    if (!std::isdigit(value[i]))
      return 1;
  }
  return 0;
}

static int isString(std::string const &value) {
  return (value.length() < 2 || value[0] != '\"' || value[value.length() - 1] != '\"' || 0);
}

static void storeRouteKeyValue(std::string const &key, std::string const &value, rt &route) {
  duplicateKey(routeKeys, key);
  if (key == "path") {
    if (isString(value))
      throw std::runtime_error("invalid path value");
    route.path() = value.substr(1, value.length() - 2);
  }
  else if (key == "source") {
    if (isString(value))
      throw std::runtime_error("invalid source value");
    route.source() = value.substr(1, value.length() - 2);
  }
  else if (key == "dictlist") {
    if (isString(value))
      throw std::runtime_error("invalid dictlist value");
    std::string temp = value.substr(1, value.length() - 2);
    if (temp != "true" && temp != "false")
      throw std::runtime_error("invalid dictlist value");
    if (temp == "true")
      route.dictlist() = true;
    else
      route.dictlist() = false;
  }
  else if (key == "redirect") {
    if (isString(value))
      throw std::runtime_error("invalid redirect value");
    route.redirect() = value.substr(1, value.length() - 2);
  }
  else if (key == "cgi_script") {
    if (isString(value))
      throw std::runtime_error("invalid cgi_script value");
    route.cgiScript() = value.substr(1, value.length() - 2);
  }
  else if (key == "cgi_interpreter") {
    if (isString(value))
      throw std::runtime_error("invalid cgi_interpreter value");
    route.cgiInterpreter() = value.substr(1, value.length() - 2);
  }
  else if (key == "cgi_timeout") {
    if (isString(value))
      throw std::runtime_error("invalid cgi_timeout value");
    std::string temp = value.substr(1, value.length() - 2);
    if (isDigit(temp))
      throw std::runtime_error("invalid cgi_timeout value");
    long long cgiTimeout = std::atoll(temp.c_str());
    if (cgiTimeout <= 0 || cgiTimeout > 9600)
      throw std::runtime_error("invalid cgi_timeout value");
    route.cgiTimeout() = static_cast<std::size_t>(cgiTimeout);
  }
  else if (key == "method") {
    if (value[0] != '[')
      throw std::runtime_error("method value must be an array");

    std::size_t pos = 1;
    if (value[pos] == ']')
      throw std::runtime_error("method array cannot be empty");

    while (pos < value.length() && value[pos] != ']') {
      if (value[pos] != '"')
        throw std::runtime_error("method array must contain strings");

      pos++;
      std::size_t methodStart = pos;

      // find closing '"'
      while (pos < value.length() && value[pos] != '"')
        pos++;

      if (pos >= value.length())
        throw std::runtime_error("unterminated string in method array");

      std::string method = value.substr(methodStart, pos - methodStart);
      if (method != "GET" && method != "POST" && method != "DELETE") {
        if (method == "PUT" || method == "PATCH" || method == "HEAD" || method == "OPTIONS")
          throw std::runtime_error("unsupported HTTP method: " + method);
        throw std::runtime_error("invalid HTTP method: " + method);
      }

      route.add(method);
      pos++; // skip closing "

      if (value[pos] == ',')
        pos++;
      else if (value[pos] == ']')
        break;
      else
        throw std::runtime_error("expected ',' or ']' in method array");
    }

    if (value[pos] != ']')
      throw std::runtime_error("expected ']' at end of method array");
  }
  else
    throw std::runtime_error("unknown routes key: " + key);
}

static int extractRouteKey(std::string const &value, std::size_t &pos, rt &route) {
  if (value[pos] != '\"')
    throw std::runtime_error("key must initiate with '\"'");

  pos++;
  std::size_t keyStart = pos;
  while (value[pos]) {
    if (value[pos] == ':')
      throw std::runtime_error("expected '\"' at the end of key");
    if (value[pos] == '\"')
      break;
    pos++;
  }
  if (value[pos] == '\0')
    throw std::runtime_error("unexpected end of file while parsing key");

  std::string key = value.substr(keyStart, pos - keyStart);
  pos += 2; // skip `":`

  if (value[pos - 1] != ':')
    throw std::runtime_error("expected ':' after key");

  std::size_t valueStart = pos;
  if (value[pos] == '[') {
    int opn = 0;
    int cls = 0;
    while (value[pos]) {
      if (value[pos] == '[')
        opn++;
      if (value[pos] == ']')
        cls++;
      if (opn == cls)
        break;
      pos++;
    }
    pos++;
  }
  else if (value[pos] == '-' || std::isdigit(value[pos])) {
    while (value[pos]) {
      if (!std::isdigit(value[pos]))
        break;
      pos++;
    }
  }
  else if (value[pos] == '\"') {
    pos++;
    while (value[pos] && value[pos] != '\"') {
      if (value[pos] == '\0')
        throw std::runtime_error("unterminated string");
      if (value[pos] == '\\' && value[pos + 1] != '\0')
        pos++;
      pos++;
    }
    pos++;
  }
  else if (value.substr(pos, 5) == "false")
    pos += 5;
  else if (value.substr(pos, 4) == "true")
    pos += 4;
  else if (value.substr(pos, 4) == "null")
    pos += 4;
  else if (value[pos] == '{') {
    int opn = 0;
    int cls = 0;
    while (value[pos]) {
      if (value[pos] == '{')
        opn++;
      if (value[pos] == '}')
        cls++;
      if (opn == cls || value[pos] == ',')
        break;
      pos++;
    }
    pos++;
  }
  else {
    while (value[pos]) {
      if (value[pos] == ',' || value[pos] == '}')
        break;
      pos++;
    }
  }

  std::string valueR = value.substr(valueStart, pos - valueStart);
  if (valueR.empty())
    throw std::runtime_error("value cannot be empty");

  storeRouteKeyValue(key, valueR, route);

  if (value[pos] != ',' && value[pos] != '}')
    throw std::runtime_error("expected ',' or '}' after key-value pair");
  if (value[pos] == ',' && value[pos + 1] != '\"')
    throw std::runtime_error("expected new key after ','");
  if (value[pos] == ',')
    pos++;
  if (value[pos] == '}')
    return 1;
  return 0;
}

static int extractRoutes(std::string const &value, std::size_t &pos, ctr &s) {
  if (value[pos] != '{')
    throw std::runtime_error("expected '{'");
  routeKeys.clear();
  rt &route = s.create();
  pos++;
  while (true) {
    if (extractRouteKey(value, pos, route))
      break;
  }
  if (value[pos] != '}')
    throw std::runtime_error("expected '}'");
  pos++;
  if (value[pos] == ',')
    pos++;
  else if (value[pos] == ']')
    return 1;
  else
    throw std::runtime_error("expected ',' or ']' after object");
  return 0;
}

static void storeKeyValue(std::string const &key, std::string const &value, ctr &s) {
  duplicateKey(serverKeys, key);
  if (key == "port") {
    if (isString(value))
      throw std::runtime_error("invalid port value");
    std::string temp = value.substr(1, value.length() - 2);
    if (isDigit(temp))
      throw std::runtime_error("invalid port value");
    long long port = std::atoll(temp.c_str());
    if (port < 1024 || port > 65535)
      throw std::runtime_error("invalid port value");
    s.port() = static_cast<std::size_t>(port);
  }
  else if (key == "name") {
    if (isString(value))
      throw std::runtime_error("invalid name value");
    s.name() = value.substr(1, value.length() - 2);
  }
  else if (key == "version") {
    if (isString(value))
      throw std::runtime_error("invalid version value");
    s.version() = value.substr(1, value.length() - 2);
  }
  else if (key == "log") {
    if (isString(value))
      throw std::runtime_error("invalid log value");
    s.log() = value.substr(1, value.length() - 2);
  }
  else if (key == "bodylimit") {
    if (isString(value))
      throw std::runtime_error("invalid bodylimit value");
    std::string temp = value.substr(1, value.length() - 2);
    if (isDigit(temp))
      throw std::runtime_error("invalid bodylimit value");
    long long bodylimit = std::atoll(temp.c_str());
    if (bodylimit < 1024 || bodylimit > 10737418240LL)
      throw std::runtime_error("invalid bodylimit value");
    s.bodylimit() = static_cast<std::size_t>(bodylimit);
  }
  else if (key == "timeout") {
    if (isString(value))
      throw std::runtime_error("invalid timeout value");
    std::string temp = value.substr(1, value.length() - 2);
    if (isDigit(temp))
      throw std::runtime_error("invalid timeout value");
    long long timeout = std::atoll(temp.c_str());
    if (timeout <= 0 || timeout > 3600000)
      throw std::runtime_error("invalid timeout value");
    s.timeout() = static_cast<std::size_t>(timeout);
  }
  else if (key == "uploaddir") {
    if (isString(value))
      throw std::runtime_error("invalid uploaddir value");
    s.uploaddir() = value.substr(1, value.length() - 2);
  }
  else if (key == "index") {
    if (isString(value))
      throw std::runtime_error("invalid index value");
    s.index() = value.substr(1, value.length() - 2);
  }
  else if (key == "root") {
    if (isString(value))
      throw std::runtime_error("invalid root value");
    s.root() = value.substr(1, value.length() - 2);
  }
  else if (key == "routes") {
    std::size_t pos = value.find("[");
    if (pos == std::string::npos || pos != 0)
      throw std::runtime_error("invalid routes format");
    pos++;
    while (true) {
      if (extractRoutes(value, pos, s))
        break;
    }
    if (value[pos] != ']')
      throw std::runtime_error("expected ']' at the end of servers array");
  }
  else if (!isDigit(key.substr(1, key.length() - 2))) {
    long long code = std::atoll(key.c_str());
    if (code < 100 || code > 599)
      throw std::runtime_error("invalid error page code: " + key);
    if (isString(value))
      throw std::runtime_error("invalid error page value");
    s.errorPages()[key] = value.substr(1, value.length() - 2);
  }
  else
    throw std::runtime_error("unknown key: " + key);
}

static int extractKey(std::string const &configFileContent, std::size_t &pos, ctr &s) {
  if (configFileContent[pos] != '\"')
    throw std::runtime_error("key must initiate with '\"'");

  pos++;
  std::size_t keyStart = pos;
  while (configFileContent[pos]) {
    if (configFileContent[pos] == ':')
      throw std::runtime_error("expected '\"' at the end of key");
    if (configFileContent[pos] == '\"')
      break;
    pos++;
  }
  if (configFileContent[pos] == '\0')
    throw std::runtime_error("unexpected end of file while parsing key");

  std::string key = configFileContent.substr(keyStart, pos - keyStart);
  pos += 2; // skip `":`

  if (configFileContent[pos - 1] != ':')
    throw std::runtime_error("expected ':' after key");

  std::size_t valueStart = pos;
  if (configFileContent[pos] == '[') {
    int opn = 0;
    int cls = 0;
    while (configFileContent[pos]) {
      if (configFileContent[pos] == '[')
        opn++;
      if (configFileContent[pos] == ']')
        cls++;
      if (opn == cls)
        break;
      pos++;
    }
    pos++;
  }
  else if (configFileContent[pos] == '-' || std::isdigit(configFileContent[pos])) {
    while (configFileContent[pos]) {
      if (!std::isdigit(configFileContent[pos]))
        break;
      pos++;
    }
  }
  else if (configFileContent[pos] == '\"') {
    pos++;
    while (configFileContent[pos] && configFileContent[pos] != '\"') {
      if (configFileContent[pos] == '\0')
        throw std::runtime_error("unterminated string");
      if (configFileContent[pos] == '\\' && configFileContent[pos + 1] != '\0')
        pos++;
      pos++;
    }
    pos++;
  }
  else if (configFileContent.substr(pos, 5) == "false")
    pos += 5;
  else if (configFileContent.substr(pos, 4) == "true")
    pos += 4;
  else if (configFileContent.substr(pos, 4) == "null")
    pos += 4;
  else if (configFileContent[pos] == '{') {
    int opn = 0;
    int cls = 0;
    while (configFileContent[pos]) {
      if (configFileContent[pos] == '{')
        opn++;
      if (configFileContent[pos] == '}')
        cls++;
      if (opn == cls || configFileContent[pos] == ',')
        break;
      pos++;
    }
    pos++;
  }
  else {
    while (configFileContent[pos]) {
      if (configFileContent[pos] == ',' || configFileContent[pos] == '}')
        break;
      pos++;
    }
  }

  std::string value = configFileContent.substr(valueStart, pos - valueStart);
  if (value.empty())
    throw std::runtime_error("value cannot be empty");

  storeKeyValue(key, value, s);

  if (configFileContent[pos] != ',' && configFileContent[pos] != '}')
    throw std::runtime_error("expected ',' or '}' after key-value pair");
  if (configFileContent[pos] == ',' && configFileContent[pos + 1] != '\"')
    throw std::runtime_error("expected new key after ','");
  if (configFileContent[pos] == ',')
    pos++;
  if (configFileContent[pos] == '}')
    return 1;
  return 0;
}

static int extractObject(std::string const &configFileContent, std::size_t &pos) {
  if (configFileContent[pos] != '{')
    throw std::runtime_error("expected '{'");
  ctr &s = server.create();
  pos++;
  while (true) {
    if (extractKey(configFileContent, pos, s))
      break;
  }
  if (configFileContent[pos] != '}')
    throw std::runtime_error("expected '}'");
  pos++;
  if (configFileContent[pos] == ',')
    pos++;
  else if (configFileContent[pos] == ']')
    return 1;
  else
    throw std::runtime_error("expected ',' or ']' after object");
  return 0;
}

void json(std::string &configFileContent) {
  skipWhitespace(configFileContent);
  std::size_t pos = configFileContent.find("{\"servers\":[");
  if (pos == std::string::npos)
    throw std::runtime_error("missing servers array");
  else if (pos != 0)
    throw std::runtime_error("unexpected token before servers array");
  pos += 12;
  while (true) {
    if (extractObject(configFileContent, pos))
      break;
    serverKeys.clear();
  }
  if (configFileContent[pos] != ']')
    throw std::runtime_error("expected ']' at the end of servers array");
  pos++;
  if (configFileContent[pos] != '}')
    throw std::runtime_error("unexpected end of file");
  fillDefault();
}