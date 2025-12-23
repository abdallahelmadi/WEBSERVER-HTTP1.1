#include <server.hpp>
#include <request.hpp>
#include <console.hpp>
#include <status.hpp>
#include <time.hpp>
#include <error.hpp>
#include <fstream>
#include <sys/socket.h>
#include <permission.hpp>
#include <sys/stat.h>

bool is_forbedden_path(const std::string& source_path, std::string valid_path) {
    struct stat buffer;
    std::cout << source_path << std::endl;
    if (stat(source_path.c_str(), &buffer) != 0) {
        std::cout << "khdam ala raso\n";
        return false;
    }
    if (S_ISREG(buffer.st_mode)) {
      
        for (size_t i = 0; i < secureFiles.size(); i++) {
            if (valid_path == secureFiles[i]) {
                return true;
            }
        }
    }
    else if (S_ISDIR(buffer.st_mode)) {
        for (size_t i = 0; i < secureFolders.size(); i++) {
            if (valid_path == secureFolders[i]) {
                return true;
            }
        }
    }

    return false;
}

void methodGet(int client, request& req, ctr& currentServer, long long startRequestTime) {

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
      bool isSecurePath = false;

      std::string fullPath = sourcePath;
      size_t pos = 0;
      std::string part;
      while((pos = fullPath.find('/', pos)) != std::string::npos) {
        part = fullPath.substr(pos);
        pos += 1;
      }
      if (!part.empty() && part[0] == '/')
        part.erase(0, 1);
      std::cout << part << std::endl;
      // check if forbedden path
      if(is_forbedden_path(sourcePath, part))
        isSecurePath = true;

      if(isSecurePath){
        // std::cout << "Path is secure, proceeding to serve the file." << std::endl;
        response = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\n" + error(403).page();
        send(client, response.c_str(), response.length(), 0);
        console.METHODS(req.getMethod(), req.getPath(), 403, time::calcl(startRequestTime, time::clock()));
        return ;
      }

      std::ifstream file;
      file.open(sourcePath.c_str());
      // std::cout << "Trying to open file: " << sourcePath << std::endl;
      if (file.is_open()) {
        // std::cout << "Serving file: " << sourcePath << std::endl;
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