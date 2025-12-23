#include <server.hpp>
#include <request.hpp>
#include <console.hpp>
#include <status.hpp>
#include <time.hpp>
#include <error.hpp>
#include <fstream>
#include <sys/socket.h>
#include <permission.hpp>


bool  isSecurePath(request &r) {

  std::vector<std::string> forbeden_files = secureFiles;
  std::vector<std::string> forbeden_folders = secureFolders;
  for (size_t i = 0; i < forbeden_files.size(); i++)
  {

    if (r.getPath().find(forbeden_files[i]) != std::string::npos)
      return true;
  }

  for (size_t i = 0; i < forbeden_folders.size(); i++)
  {
    if (r.getPath().find("/" + forbeden_folders[i]) != std::string::npos)
      return true;
    std::cout << "Checking folder: " << forbeden_folders[i] << std::endl;
    std::cout << "Folder not found in path." << std::endl;
  }
  return false;
}


void methodGet(int client, request& req, ctr& currentServer, long long startRequestTime) {

  std::cout << "Received GET request for path: " << req.getPath() << std::endl;

  std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + error(404).page();

  std::string notFoundPageSource = currentServer.errorPages()["404"];  
  if (!notFoundPageSource.empty()) {
    std::stringstream body;
    std::fstream file(notFoundPageSource.c_str());
    if(file.is_open()){
      body << file.rdbuf();
      response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + body.str();
    }
  }

  std::size_t i = 0;
  while (i < currentServer.length()) {
    std::string sourcePath = currentServer.route(i).source();
    if (req.getPath() == currentServer.route(i).path()) {
      if(isSecurePath(req) == true){
        std::cout << "Path is secure, proceeding to serve the file." << std::endl;
        response = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\n" + error(403).page();
        send(client, response.c_str(), response.length(), 0);
        console.METHODS(req.getMethod(), req.getPath(), 403, time::calcl(startRequestTime, time::clock()));
        return ;
      }

      std::ifstream file;
      file.open(sourcePath.c_str());
      std::cout << "Trying to open file: " << sourcePath << std::endl;
      if (file.is_open()) {
        std::cout << "Serving file: " << sourcePath << std::endl;
        std::stringstream body;
        body << file.rdbuf();
        file.close();
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + body.str();
        send(client, response.c_str(), response.length(), 0);
        console.METHODS(req.getMethod(), req.getPath(), 200, time::calcl(startRequestTime, time::clock()));
        return;
      }
      break;
    }
    i++;
  }
  send(client, response.c_str(), response.length(), 0);
  console.METHODS(req.getMethod(), req.getPath(), 404, time::calcl(startRequestTime, time::clock()));
}