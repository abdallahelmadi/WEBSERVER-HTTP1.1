#include <server.hpp>
#include <request.hpp>
#include <console.hpp>
#include <status.hpp>
#include <type.hpp>
#include <time.hpp>
#include <response.hpp>
#include <path.hpp>
#include <error.hpp>
#include <fstream>
#include <sys/socket.h>
#include <permission.hpp>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <dirent.h>
#include <sys/wait.h>
#include <client.hpp>
#include <fcntl.h>


#define FILE_STREAM_THRESHOLD 1048576 //1Mb

std::string methodGet(int client, request& req, ctr& currentServer, long long startRequestTime, Client &clientObj) {

  // find matching route at config file
  rt* route = NULL;
  for (std::size_t i = 0; i < currentServer.length(); i++) {
    if (path::check(currentServer.route(i).path(), req.getPath())) {
      route = &currentServer.route(i);
      break;
    }
  }

  std::string sourcePathToHandle;

  if (!route) {
    // absolute path
    sourcePathToHandle = currentServer.root() + req.getPath();

    // check if file exists
    struct stat fileStat;
    if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
      // 404 not found
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
    }

    // check if it's a directory
    if (S_ISDIR(fileStat.st_mode))
      sourcePathToHandle = sourcePathToHandle + "/" + currentServer.index();
  } else {
    sourcePathToHandle = route->source();

    // 405 method not allowed
    if (
      route->method(0) != "GET" &&
      (route->length() > 1 && route->method(1) != "GET") &&
      (route->length() > 2 && route->method(2) != "GET")
    ) {
      std::map<std::string, std::string> Theaders;
      Theaders["Allow"] = "GET, POST, DELETE";
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 405, Theaders, "", req, currentServer).sendResponse();
    }

    if (route->cgiScript().empty() && route->dictlist() == false && route->redirect().empty()) {
      struct stat fileStat;
      if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
        std::map<std::string, std::string> Theaders;
        Theaders["Content-Type"] = "text/html";
        return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
      }

      if (S_ISDIR(fileStat.st_mode))
        sourcePathToHandle = sourcePathToHandle + "/" + currentServer.index();
    }
  }

  // handle redirections
  if (route && !route->redirect().empty()) {
    std::map<std::string, std::string> Theaders;
    Theaders["Location"] = route->redirect();
    Theaders["Cache-Control"] = "no-store";
    return response(client, startRequestTime, (route->redirect().find("http") == 0 ? 302 : 301), Theaders, "", req, currentServer).sendResponse();
  }

  // handle dictlist
  if (route && route->dictlist()) {
    struct stat fileStat;
    if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
    }

    if (S_ISDIR(fileStat.st_mode)) {
      DIR* appDir = opendir(sourcePathToHandle.c_str());
      if (!appDir) {
        console.issue("cannot open directory");
        return "";
      }

      struct dirent* contentTemp;
      std::stringstream body;
      while ((contentTemp = readdir(appDir)) != NULL) {
        std::string name = contentTemp->d_name;
        if (name == "." || name == "..")
          continue;
        body << "<a href=\"" << route->path() << "/" << name << "\">" << name << "</a><br/>";
      }
      closedir(appDir);
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 200, Theaders, body.str(), req, currentServer).sendResponse();
    }
  }

  // handle cgi execution (use original method - CGI is typically small)
  // if (route && !route->cgiScript().empty()) {
  //   return methodGet(client, req, currentServer, startRequestTime);
  // }

  // =========================================================================
  // FILE SERVING - Check if file should be streamed
  // =========================================================================
  struct stat fileStat;
  if (stat(sourcePathToHandle.c_str(), &fileStat) != 0) {
    std::map<std::string, std::string> Theaders;
    Theaders["Content-Type"] = "text/html";
    return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
  }

  // Read the file content
  if (stat(sourcePathToHandle.c_str(), &fileStat) != 0) {
    std::map<std::string, std::string> Theaders;
    Theaders["Content-Type"] = "text/html";
    return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
  }

  // For large files (> 1MB), use streaming
  if (fileStat.st_size > FILE_STREAM_THRESHOLD) {
    // Open file for streaming
    int fd = open(sourcePathToHandle.c_str(), O_RDONLY);
    if (fd < 0) {
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
    }

    // Setup client for streaming
    clientObj.fd_file = fd;
    clientObj.file_size = fileStat.st_size;
    clientObj.file_offset = 0;
    clientObj.header_sent = false;
    clientObj.is_streaming = true;

    // Build HTTP headers only (body will be streamed)
    std::stringstream headers;
    headers << "HTTP/1.1 200 OK\r\n";
    headers << "Content-Type: " << type::get(sourcePathToHandle) << "\r\n";
    headers << "Content-Length: " << fileStat.st_size << "\r\n";
    headers << "\r\n";

    console.METHODS(req.getMethod(), req.getPath(), 200, time::calcl(startRequestTime, time::clock()));
    // std::map<std::string, std::string> Theaders;
    // Theaders["Content-Type"] = type::get(sourcePathToHandle);
    return headers.str();
  }
  std::ifstream file(sourcePathToHandle.c_str(), std::ios::binary);
  if (!file.is_open()) {
    std::map<std::string, std::string> Theaders;
    Theaders["Content-Type"] = "text/html";
    return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
  }

  std::stringstream fileContent;
  fileContent << file.rdbuf();
  file.close();

  std::map<std::string, std::string> Theaders;
  Theaders["Content-Type"] = type::get(sourcePathToHandle);
  return response(client, startRequestTime, 200, Theaders, fileContent.str(), req, currentServer).sendResponse();
}
// add some variables to the client.hpp to store file descriptor of the big file
// response will be just the headers
// then the file descriptor of the opened file
// and the current position in the file to be sent in the next write event
// and flags to indicate if the headers have been sent or not
// badr work -----------------------------------------------
// then modify the handle_write_event function to check if there is a file to be sent
// if there is, read a chunk of the file and send it to the client
// if the whole file has been sent, close the file descriptor and reset the variables 
