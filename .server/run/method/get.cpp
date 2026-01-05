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
#include <cstdlib>
#include <fcntl.h>
#include <client.hpp>

// Size threshold for streaming (1MB) - larger files are streamed
#define FILE_STREAM_THRESHOLD 1048576

std::string generate_session_id(size_t length)
{
    static const char hex[] = "0123456789abcdef";
    std::string id;
    for (size_t i = 0; i < length; ++i)
    {
        id += hex[std::rand() % 16];
    }
    return id;
}

std::string methodGet(int client, request& req, ctr& currentServer, long long startRequestTime) {

  // find matching route at config file
  rt* route = NULL;
  for (std::size_t i = 0; i < currentServer.length(); i++) {
    if (path::check(currentServer.route(i).path(), req.getPath())) {
      route = &currentServer.route(i);
      break;
    }
  }

  std::string sourcePathToHandle;
  // check if we have a cookie for sestion management
  std::map<std::string, std::string> headers = req.getHeaders();
  // std::map<std::string, std::string>::iterator it = headers.find("Cookie");

  if (!route) {
    // absolute path
    sourcePathToHandle = currentServer.root() + req.getPath();
    // Automatic index resolution for GET `server.index()`

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
      // Automatic index resolution for GET `server.index()`
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
      // 404 not found
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
    }

    // check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {

      // open web application dir
      DIR* appDir = opendir(sourcePathToHandle.c_str());
      if (!appDir) {
        console.issue("cannot open directory");
        return "";
      }

      struct dirent* contentTemp;
      std::stringstream body;

      // read each time directory/file
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

  // handle cgi execution
  if (route && !route->cgiScript().empty()) {

    if (route->cgiInterpreter().empty()) {
      // 500 internal server error
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
    }

    if (permission::check(route->cgiScript())) {
      // 404 not found
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
    }

    // check if it's a directory
    struct stat fileStat;
    if (stat(route->cgiScript().c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode)) {
      // 404 not found
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
    }

    int pipeFD[2];
    if (pipe(pipeFD) == -1) {
      // 500 internal server error
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
    }

    pid_t pid = fork();
    if (pid < 0) {
      // 500 internal server error
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
    }

    if (pid == 0) {
      // child process
      close(pipeFD[0]); // close read end
      dup2(pipeFD[1], 1); // redirect stdout to pipe write end
      extern char** environ;
      char* argv[3];
      argv[0] = const_cast<char*>(route->cgiInterpreter().c_str());
      argv[1] = const_cast<char*>(route->cgiScript().c_str());
      argv[2] = NULL;
      execve(argv[0], argv, environ);
      _exit(127);
    } else {

      // parent process
      close(pipeFD[1]); // close write end
      int status = 0;
      int code = 0;
      waitpid(pid, &status, 0); // wait for child process

      if (WIFEXITED(status))
        code = WEXITSTATUS(status);
      else if (WIFSIGNALED(status))
        code = WTERMSIG(status);

      if (code != 0) {
        // 500 internal server error
        std::map<std::string, std::string> Theaders;
        Theaders["Content-Type"] = "text/html";
        return response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
      }

      char buffer[1024];
      std::stringstream cgiOutput;
      ssize_t bytesRead;
      while ((bytesRead = read(pipeFD[0], buffer, sizeof(buffer))) > 0) {
        cgiOutput.write(buffer, bytesRead);
      }
      close(pipeFD[0]); // close read end

      if (bytesRead < 0) {
        // 500 internal server error
        std::map<std::string, std::string> Theaders;
        Theaders["Content-Type"] = "text/html";
        return response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
      }

      if (route->cgiTimeout() > 0 && time::calcl(startRequestTime, time::clock()) > static_cast<long long>(route->cgiTimeout())) {
        // 504 gateway timeout
        std::map<std::string, std::string> Theaders;
        Theaders["Content-Type"] = "text/html";
        return response(client, startRequestTime, 504, Theaders, "", req, currentServer).sendResponse();
      }

      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 200, Theaders, cgiOutput.str(), req, currentServer).sendResponse();
    }

    return "";
  }

  // Read the file content
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

// ============================================================================
// STREAMING VERSION OF GET METHOD
// Handles large files (videos, etc.) by streaming them in chunks
// This prevents blocking the server for other clients
// ============================================================================
// std::string methodGetStreaming(int client, request& req, ctr& currentServer, long long startRequestTime, Client& clientObj) {

//   // find matching route at config file
//   rt* route = NULL;
//   for (std::size_t i = 0; i < currentServer.length(); i++) {
//     if (path::check(currentServer.route(i).path(), req.getPath())) {
//       route = &currentServer.route(i);
//       break;
//     }
//   }

//   std::string sourcePathToHandle;

//   if (!route) {
//     // absolute path
//     sourcePathToHandle = currentServer.root() + req.getPath();

//     // check if file exists
//     struct stat fileStat;
//     if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
//       // 404 not found
//       std::map<std::string, std::string> Theaders;
//       Theaders["Content-Type"] = "text/html";
//       return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
//     }

//     // check if it's a directory
//     if (S_ISDIR(fileStat.st_mode))
//       sourcePathToHandle = sourcePathToHandle + "/" + currentServer.index();
//   } else {
//     sourcePathToHandle = route->source();

//     // 405 method not allowed
//     if (
//       route->method(0) != "GET" &&
//       (route->length() > 1 && route->method(1) != "GET") &&
//       (route->length() > 2 && route->method(2) != "GET")
//     ) {
//       std::map<std::string, std::string> Theaders;
//       Theaders["Allow"] = "GET, POST, DELETE";
//       Theaders["Content-Type"] = "text/html";
//       return response(client, startRequestTime, 405, Theaders, "", req, currentServer).sendResponse();
//     }

//     if (route->cgiScript().empty() && route->dictlist() == false && route->redirect().empty()) {
//       struct stat fileStat;
//       if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
//         std::map<std::string, std::string> Theaders;
//         Theaders["Content-Type"] = "text/html";
//         return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
//       }

//       if (S_ISDIR(fileStat.st_mode))
//         sourcePathToHandle = sourcePathToHandle + "/" + currentServer.index();
//     }
//   }

//   // handle redirections
//   if (route && !route->redirect().empty()) {
//     std::map<std::string, std::string> Theaders;
//     Theaders["Location"] = route->redirect();
//     Theaders["Cache-Control"] = "no-store";
//     return response(client, startRequestTime, (route->redirect().find("http") == 0 ? 302 : 301), Theaders, "", req, currentServer).sendResponse();
//   }

//   // handle dictlist
//   if (route && route->dictlist()) {
//     struct stat fileStat;
//     if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
//       std::map<std::string, std::string> Theaders;
//       Theaders["Content-Type"] = "text/html";
//       return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
//     }

//     if (S_ISDIR(fileStat.st_mode)) {
//       DIR* appDir = opendir(sourcePathToHandle.c_str());
//       if (!appDir) {
//         console.issue("cannot open directory");
//         return "";
//       }

//       struct dirent* contentTemp;
//       std::stringstream body;
//       while ((contentTemp = readdir(appDir)) != NULL) {
//         std::string name = contentTemp->d_name;
//         if (name == "." || name == "..")
//           continue;
//         body << "<a href=\"" << route->path() << "/" << name << "\">" << name << "</a><br/>";
//       }
//       closedir(appDir);
//       std::map<std::string, std::string> Theaders;
//       Theaders["Content-Type"] = "text/html";
//       return response(client, startRequestTime, 200, Theaders, body.str(), req, currentServer).sendResponse();
//     }
//   }

//   // handle cgi execution (use original method - CGI is typically small)
//   if (route && !route->cgiScript().empty()) {
//     return methodGet(client, req, currentServer, startRequestTime);
//   }

//   // =========================================================================
//   // FILE SERVING - Check if file should be streamed
//   // =========================================================================
//   struct stat fileStat;
//   if (stat(sourcePathToHandle.c_str(), &fileStat) != 0) {
//     std::map<std::string, std::string> Theaders;
//     Theaders["Content-Type"] = "text/html";
//     return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
//   }

//   // For large files (> 1MB), use streaming
//   if (fileStat.st_size > FILE_STREAM_THRESHOLD) {
//     // Open file for streaming
//     int fd = open(sourcePathToHandle.c_str(), O_RDONLY);
//     if (fd < 0) {
//       std::map<std::string, std::string> Theaders;
//       Theaders["Content-Type"] = "text/html";
//       return response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
//     }

//     // Set non-blocking mode
//     fcntl(fd, F_SETFL, O_NONBLOCK);

//     // Setup client for streaming
//     clientObj.file_fd = fd;
//     clientObj.file_size = fileStat.st_size;
//     clientObj.file_sent = 0;
//     clientObj.headers_sent = false;
//     clientObj.is_streaming_file = true;

//     // Build HTTP headers only (body will be streamed)
//     std::stringstream headers;
//     headers << "HTTP/1.1 200 OK\r\n";
//     headers << "Content-Type: " << type::get(sourcePathToHandle) << "\r\n";
//     headers << "Content-Length: " << fileStat.st_size << "\r\n";
//     headers << "Accept-Ranges: bytes\r\n";
//     headers << "Connection: close\r\n";
//     headers << "\r\n";

//     console.METHODS(req.getMethod(), req.getPath(), 200, time::calcl(startRequestTime, time::clock()));
//     return headers.str();
//   }

//   // For small files, read into memory (original behavior)
//   std::ifstream file(sourcePathToHandle.c_str(), std::ios::binary);
//   if (!file.is_open()) {
//     std::map<std::string, std::string> Theaders;
//     Theaders["Content-Type"] = "text/html";
//     return response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
//   }

//   std::stringstream fileContent;
//   fileContent << file.rdbuf();
//   file.close();

//   std::map<std::string, std::string> Theaders;
//   Theaders["Content-Type"] = type::get(sourcePathToHandle);
//   return response(client, startRequestTime, 200, Theaders, fileContent.str(), req, currentServer).sendResponse();
// }
// add some variables to the client.hpp to store file descriptor of the big file
// response will be just the headers
// then the file descriptor of the opened file
// and the current position in the file to be sent in the next write event
// and flags to indicate if the headers have been sent or not
// badr work -----------------------------------------------
// then modify the handle_write_event function to check if there is a file to be sent
// if there is, read a chunk of the file and send it to the client
// if the whole file has been sent, close the file descriptor and reset the variables 
