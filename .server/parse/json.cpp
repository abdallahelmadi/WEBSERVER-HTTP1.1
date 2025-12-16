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
      server[i].name() = "app/" + server[i].name();
    if (server[i].version().empty())
      server[i].version() = "0.1.0";
    if (server[i].root().empty())
      server[i].root() = "./";

    if (server[i].notfound().empty())
      server[i].notfound() = ".server/.build/not-found.html";
    else
      server[i].notfound() = "app/" + server[i].name() + "/" + server[i].root() + "/" + server[i].notfound();

    if (server[i].servererror().empty())
      server[i].servererror() = "app/" + server[i].name() + "/" + server[i].root() + "/" + server[i].servererror();
    if (server[i].log().empty())
      server[i].log() = ".server/.log/" + server[i].name() + "/" + server[i].name() + ".log";
    else
      server[i].log() = "app/" + server[i].name() + server[i].root() + "/" + server[i].log();
    if (!server[i].bodylimit())
      server[i].bodylimit() = 1048576;
    if (!server[i].timeout())
      server[i].timeout() = 30000;
    if (server[i].uploaddir().empty())
      server[i].uploaddir() = "app/" + server[i].name() + "/uploads";
    else
      server[i].uploaddir() = "app/" + server[i].name() + "/" + server[i].root() + "/" + server[i].uploaddir();
    if (server[i].index().empty())
      server[i].index() = "index.html";
    if (server[i].length() == 0)
      throw std::runtime_error("routes are required for each server");
    std::vector<std::string> tempPaths;
    for (std::size_t j = 0; j < server[i].length(); j++) {
      tempPaths.push_back(server[i].route(j).path());
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
      if (route.path().empty())
        throw std::runtime_error("route path is required");
      if (route.source().empty())
        throw std::runtime_error("route source is required");

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
  std::vector<std::string> temp;
  for (std::size_t i = 0; i < server.length(); i++) {
    temp.push_back(server[i].name());
  }
  for (std::size_t i = 0; i < temp.size(); i++) {
    for (std::size_t j = i + 1; j < temp.size(); j++) {
      if (temp[i] == temp[j])
        throw std::runtime_error("duplicate server name: " + temp[i]);
    }
  }
  temp.clear();
}

std::vector<std::string> serverKeys;
std::vector<std::string> routeKeys;

static void skipWhitespace(std::string &value) {
  std::size_t index = 0;
  while (value[index]) {
    if (value[index] == ' ' || value[index] == '\n' || value[index] == '\r' ||
        value[index] == '\t' || value[index] == '\v' || value[index] == '\f')
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
      if (method != "GET" && method != "POST" && method != "POST") {
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
    throw std::runtime_error("unknown key: " + key);
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
  rt &route = s.create();
  pos++;
  while (true) {
    if (extractRouteKey(value, pos, route))
      break;
    routeKeys.clear();
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
    if (isDigit(value))
      throw std::runtime_error("invalid port value");
    int port = std::atoi(value.c_str());
    if (port < 1024 || port > 65535)
      throw std::runtime_error("port must be between 1024 and 65535");
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
  else if (key == "notfound") {
    if (isString(value))
      throw std::runtime_error("invalid notfound value");
    s.notfound() = value.substr(1, value.length() - 2);
  }
  else if (key == "servererror") {
    if (isString(value))
      throw std::runtime_error("invalid servererror value");
    s.servererror() = value.substr(1, value.length() - 2);
  }
  else if (key == "log") {
    if (isString(value))
      throw std::runtime_error("invalid log value");
    s.log() = value.substr(1, value.length() - 2);
  }
  else if (key == "bodylimit") {
    if (isDigit(value))
      throw std::runtime_error("invalid bodylimit value");
    int bodylimit = std::atoi(value.c_str());
    if (bodylimit < 0)
      throw std::runtime_error("bodylimit must be a non-negative number");
    s.bodylimit() = static_cast<std::size_t>(bodylimit);
  }
  else if (key == "timeout") {
    if (isDigit(value))
      throw std::runtime_error("invalid timeout value");
    int timeout = std::atoi(value.c_str());
    if (timeout < 0)
      throw std::runtime_error("timeout must be a non-negative number");
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