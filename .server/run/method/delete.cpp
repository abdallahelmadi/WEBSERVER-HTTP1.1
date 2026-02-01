#include <server.hpp>
#include <request.hpp>
#include <response.hpp>
#include <error.hpp>
#include <console.hpp>
#include <type.hpp>
#include <status.hpp>
#include <path.hpp>
#include <time.hpp>
#include <fstream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>

std::string methodDelete(int client, request& req, ctr& currentServer, long long startRequestTime) {

  rt* route = NULL;
  for (std::size_t i = 0; i < currentServer.length(); i++) {
    if (path::check(currentServer.route(i).path(), req.getPath())) {
      route = &currentServer.route(i);
      break;
    }
  }

  std::string sourcePathToDelete;

  if (!route) {
     sourcePathToDelete = currentServer.root() + req.getPath();
    // There is NO automatic index resolution for DELETE
  } else {
    sourcePathToDelete = route->source();}

  for (std::size_t i = 0; i < route->length(); i++) {
    if (route->method(i) == "DELETE") {
      break;
    }
    if (i == route->length() - 1) {
      std::map<std::string, std::string> Theaders;
      Theaders["Allow"] = "GET, POST, DELETE";
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 405, Theaders, "", req, currentServer).sendResponse();
    }
  }

  // check if file exists
  struct stat fileStat;
  if (stat(sourcePathToDelete.c_str(), &fileStat) != 0) {
    // 404 not found
    std::map<std::string, std::string> Theaders;
    Theaders["Content-Type"] = "text/html";
    return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
  }

  // check if it's a directory
  if (S_ISDIR(fileStat.st_mode)) {
    // 403 forbidden
    std::map<std::string, std::string> Theaders;
    Theaders["Content-Type"] = "text/html";
    return response(client, startRequestTime, 403, Theaders, "", req, currentServer).sendResponse();
  }

  // else file, try to delete the file
  if (unlink(sourcePathToDelete.c_str()) == 0) {
    // 204 no content
    std::map<std::string, std::string> Theaders;
    return response(client, startRequestTime, 204, Theaders, "", req, currentServer).sendResponse();
  } else {
    // 500 internal server error
    std::map<std::string, std::string> Theaders;
    Theaders["Content-Type"] = "text/html";
    return response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
  }
  return "";
}