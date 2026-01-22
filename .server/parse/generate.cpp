#include <dirent.h>
#include <netdb.h>
#include <server.hpp>
#include <sstream>
#include <console.hpp>
#include <permission.hpp>
#include <vector>

static bool includes(std::vector<std::string> const& vec, std::string const& item) {
  for (std::size_t i = 0; i < vec.size(); i++) {
    if (vec[i] == item)
      return true;
  }
  return false;
}

static void createRoutes(std::string fullPath, ctr& currentServer, std::string basePath = "/") {

  // open web application dir
  DIR* appDir = opendir(fullPath.c_str());
  if (!appDir)
    return;

  struct dirent* contentTemp;

  // read each time directory/file
  while ((contentTemp = readdir(appDir)) != NULL) {
    std::string name = contentTemp->d_name;
    if (name == "." || name == "..")
      continue;
    if (contentTemp->d_type == DT_DIR) {
      // this is a directory
      if (!includes(secureFolders, name))
        createRoutes(fullPath + name + "/", currentServer, basePath + name + "/");
    }
    else if (contentTemp->d_type == DT_REG) {
      // this is a file
      if (!includes(secureFiles, name)) {
        // this file can be a route
        rt& route = currentServer.create();
        route.path() = basePath + name;
        route.source() = fullPath + name;
        route.add("GET");
        route.add("POST");
      }
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

  // set name
  currentServer.name() = name;
  // set default values
  currentServer.port() = defaultPort;
  defaultPort++;
  currentServer.version() = "0.1.0";
  currentServer.log() = ".server/.log/" + currentServer.name() + "/" + currentServer.name() + ".log";
  currentServer.bodylimit() = 1048576;
  currentServer.timeout() = 30000;
  currentServer.index() = "index.html";
  currentServer.root() = "app/" + currentServer.name() + "/";
  currentServer.uploaddir() = currentServer.root() + "/uploads";

  // read directory and make routes
  std::string fullPath = "app/" + name + "/";
  createRoutes(fullPath, currentServer);
}

void generate(void) {

  // open app directory
  DIR* appDir = opendir("app");
  if (!appDir)
    throw std::runtime_error("could not open app directory");

  std::size_t appSize = 0;
  std::vector<std::string> appArray;
  struct dirent* contentTemp;

  // read each time directory/file
  while ((contentTemp = readdir(appDir)) != NULL) {
    std::string name = contentTemp->d_name;
    if (name == "." || name == "..")
      continue;
    if (contentTemp->d_type == DT_DIR) {
      // this is a directory
      appSize++;
      appArray.push_back(name);
    }
    else if (contentTemp->d_type == DT_REG) {
      // this is a file
      closedir(appDir);
      throw std::runtime_error("app directory cannot contain files");
    }
  }

  // check number of directories
  if (appSize <= 0)
    throw std::runtime_error("no web applications found in app directory");

  closedir(appDir);

  // loop each folder and create server for each application
  for (std::size_t i = 0; i < appArray.size(); i++) {
    createServer(appArray[i]);
  }
}