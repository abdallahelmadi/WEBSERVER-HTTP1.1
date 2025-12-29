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

void methodGet(int client, request& req, ctr& currentServer, long long startRequestTime) {

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

    // check if file exists
    struct stat fileStat;
    if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
      // 404 not found
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
      return;
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
      response(client, startRequestTime, 405, Theaders, "", req, currentServer).sendResponse();
      return;
    }

    if (route->cgiScript().empty() && route->dictlist() == false && route->redirect().empty()) {
      // Automatic index resolution for GET `server.index()`
      // check if file exists
      struct stat fileStat;
      if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
        // 404 not found
        std::map<std::string, std::string> Theaders;
        Theaders["Content-Type"] = "text/html";
        response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
        return;
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
    response(client, startRequestTime, (route->redirect().find("http") == 0 ? 302 : 301), Theaders, "", req, currentServer).sendResponse();
    return;
  }

  // handle dictlist
  if (route && route->dictlist()) {

    struct stat fileStat;
    if (stat(sourcePathToHandle.c_str(), &fileStat) != 0 || permission::check(sourcePathToHandle)) {
      // 404 not found
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
      return;
    }

    // check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {

      // open web application dir
      DIR* appDir = opendir(sourcePathToHandle.c_str());
      if (!appDir) {
        console.issue("cannot open directory");
        return;
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
      response(client, startRequestTime, 200, Theaders, body.str(), req, currentServer).sendResponse();
      return;
    }
  }

  // handle cgi execution
  if (route && !route->cgiScript().empty()) {

    if (route->cgiInterpreter().empty()) {
      // 500 internal server error
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
      return;
    }

    if (permission::check(route->cgiScript())) {
      // 404 not found
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
      return;
    }

    // check if it's a directory
    struct stat fileStat;
    if (stat(route->cgiScript().c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode)) {
      // 404 not found
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
      return;
    }

    int pipeFD[2];
    if (pipe(pipeFD) == -1) {
      // 500 internal server error
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
      return;
    }

    pid_t pid = fork();
    if (pid < 0) {
      // 500 internal server error
      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
      return;
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

      if (status != 0) {
        // 500 internal server error
        std::map<std::string, std::string> Theaders;
        Theaders["Content-Type"] = "text/html";
        response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
        return;
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
        response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
        return;
      }

      if (route->cgiTimeout() > 0 && time::calcl(startRequestTime, time::clock()) > static_cast<long long>(route->cgiTimeout())) {
        // 504 gateway timeout
        std::map<std::string, std::string> Theaders;
        Theaders["Content-Type"] = "text/html";
        response(client, startRequestTime, 504, Theaders, "", req, currentServer).sendResponse();
        return;
      }

      std::map<std::string, std::string> Theaders;
      Theaders["Content-Type"] = "text/html";
      response(client, startRequestTime, 200, Theaders, cgiOutput.str(), req, currentServer).sendResponse();
    }

    return;
  }

  std::map<std::string, std::string> Theaders;
  Theaders["Content-Type"] = type::get(sourcePathToHandle);
  response(client, startRequestTime, 1337, Theaders, "", req, currentServer).sendGETchunks(sourcePathToHandle);
  return;
}