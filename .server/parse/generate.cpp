#include <dirent.h>
#include <netdb.h>
#include <server.hpp>
#include <sstream>
#include <console.hpp>
#include <vector>

static void createRoutes(std::string fullPath, ctr& currentServer, std::string basePath = "/") {

  // open web application dir
  DIR* appDir = opendir(fullPath.c_str());
  if (!appDir)
    return;

  struct dirent* contentTemp;

  // read each time directory/file
  while ((contentTemp = readdir(appDir)) != NULL) {
    std::string name = contentTemp->d_name;
    if (name[0] == '.')
      continue;
    if (contentTemp->d_type == DT_DIR) {
      createRoutes(fullPath + name + "/", currentServer, basePath + name + "/");
    }
    else if (contentTemp->d_type == DT_REG) {
      rt& route = currentServer.create();
      route.path() = basePath + name;
      route.source() = fullPath + name;
      route.add("GET");
      route.add("POST");
    }
  }

  rt& route = currentServer.create();
  route.path() = basePath;
  route.source() = fullPath + currentServer.index();
  route.add("GET");
  route.add("POST");

  closedir(appDir);
}

std::size_t defaultPort = 3000;
static void createServer(std::string name) {
  ctr& currentServer = server.create();
  currentServer.name() = name;
  currentServer.port() = defaultPort;
  defaultPort++;
  currentServer.version() = "0.1.0";
  currentServer.index() = "index.html";
  currentServer.root() = "app/" + currentServer.name() + "/";
  std::string fullPath = "app/" + name + "/";
  createRoutes(fullPath, currentServer);
}

void generate(void) {

  DIR* appDir = opendir("app");
  if (!appDir)
    throw std::runtime_error("could not open app directory");

  std::size_t appSize = 0;
  std::vector<std::string> appArray;
  struct dirent* contentTemp;

  while ((contentTemp = readdir(appDir)) != NULL) {
    std::string name = contentTemp->d_name;
    if (name == "." || name == "..")
      continue;
    if (contentTemp->d_type == DT_DIR) {
      appSize++;
      appArray.push_back(name);
    }
    else if (contentTemp->d_type == DT_REG) {
      closedir(appDir);
      throw std::runtime_error("app directory cannot contain files");
    }
  }

  if (appSize <= 0)
    throw std::runtime_error("no web applications found in app directory");

  closedir(appDir);

  for (std::size_t i = 0; i < appArray.size(); i++) {
    createServer(appArray[i]);
  }
}