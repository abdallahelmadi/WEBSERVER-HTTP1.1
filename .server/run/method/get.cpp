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
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <dirent.h>
#include <sys/wait.h>
#include <client.hpp>
#include <fcntl.h>

#define FILE_STREAM_THRESHOLD 1048576

// add a function to check if the user reqsueted a  profile page while not being logged in
bool isProfilePageRequestWithoutLogin(request& req, Client& clientObj) {
  if (path::check(req.getPath(), "/profile") && clientObj._has_logged_in == false) {
    return true;
  }
  return false;
}

// add a function to check if the user requested the login page while already being logged in
bool isLoginPageRequest_but_already_Login(request& req, Client& clientObj) {
  if (path::check(req.getPath(), "/login") && clientObj._has_logged_in == true) {
    return true;
  }
  return false;
}

// add function to check if the cookie header is messing when the user is logged in
bool isCookieHeaderMissingWhenLoggedIn(request& req, Client& clientObj) {
  
  std::map<std::string, std::string> headers = req.getHeaders();
  // check if we have a cokie header when the user is logged in
  if(path::check(req.getPath(), "/profile") && headers.find("Cookie") == headers.end() && clientObj._has_logged_in == true) {
      clientObj._has_logged_in = false; //force logout
      return true;
  }
  return false;
}

bool requestforlogout(request& req, Client& clientObj) {
  if (path::check(req.getPath(), "/logout")) {
    clientObj._has_logged_in = false;
    return true;
  }
  return false;
}

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
    // Automatic index resolution for GET `server.index()`
    //check if the user wants to logout !
    if(requestforlogout(req, clientObj)) {
      std::map<std::string, std::string> headers;
      headers["location"] = "/login?logged_out=1";
      headers["Set-Cookie"] = "sessionid=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0; Path=/";
      headers["Cache-Control"] = "no-store";
      return response(client, startRequestTime, 302, headers, "", req, currentServer).sendResponse();
    }
    // check if file exists
    struct stat fileStat;
    if (stat(sourcePathToHandle.c_str(), &fileStat) != 0) {
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

  for (std::size_t i = 0; i < route->length(); i++) {
    if (route->method(i) == "GET") {
      break;
    }
    if (i == route->length() - 1) {
      std::map<std::string, std::string> Theaders;
      Theaders["Allow"] = "GET, POST, DELETE";
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 405, Theaders, "", req, currentServer).sendResponse();
    }
  }

    if (route->cgiScript().empty() && route->dictlist() == false && route->redirect().empty()) {
      struct stat fileStat;
      if (stat(sourcePathToHandle.c_str(), &fileStat) != 0) {
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
    if (stat(sourcePathToHandle.c_str(), &fileStat) != 0) {
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
    }

    std::map<std::string, std::string> Theaders;
    Theaders["Content-Type"] = "text/html";

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
        if (name[0] == '.')
          continue;
        body << "<a href='" << route->path() << "/" + name << "'>" << name << "</a><br/>";
      }
      closedir(appDir);
      return response(client, startRequestTime, 200, Theaders, body.str(), req, currentServer).sendResponse();
    }
  }

  struct stat fileStat;
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

  if(isProfilePageRequestWithoutLogin(req, clientObj)) {
    std::map<std::string, std::string> headers;
    headers["location"] = "/login?need_login=1";
    headers["Cache-Control"] = "no-store";
    return response(client, startRequestTime, 302, headers, "", req, currentServer).sendResponse();
  }
  if(isCookieHeaderMissingWhenLoggedIn(req, clientObj)) {
    // std::cout << "Forcing logout due to missing Cookie header." << std::endl;
    std::map<std::string, std::string> headers;
    headers["location"] = "/login?need_login=1";
    headers["Cache-Control"] = "no-store";
    return response(client, startRequestTime, 302, headers, "", req, currentServer).sendResponse();
  }
  if(isLoginPageRequest_but_already_Login(req, clientObj)) {
    std::map<std::string, std::string> headers;
    headers["location"] = "/profile?already_logged_in=1";
    headers["Cache-Control"] = "no-store";
    return response(client, startRequestTime, 302, headers, "", req, currentServer).sendResponse();
  }

  std::map<std::string, std::string> Theaders;
  Theaders["Content-Type"] = type::get(sourcePathToHandle);
  return response(client, startRequestTime, 200, Theaders, fileContent.str(), req, currentServer).sendResponse();
}
