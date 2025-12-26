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
#include <unistd.h>
#include <string>


bool is_forbedden_path(const std::string& source_path, std::string valid_path) {
    struct stat buffer;
    if (stat(source_path.c_str(), &buffer) != 0)
        return false;
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

bool  is_dir(std::string const & source_path) {
  struct stat buffer;
  if (stat(source_path.c_str(), &buffer) != 0)
        return false;
  return S_ISDIR(buffer.st_mode);
}

std::string get_the_Content_Type(const std::string path){

  size_t pos = 0;
  // std::cout << "path: " << path << std::endl;
  if((pos = path.find(".")) != std::string::npos){
    std::string content_path = path.substr(pos, path.length() - pos);
    //check if an image !!
    if(content_path == ".jpg")
      return "Content-Type: image/jpg";
    else if (content_path == ".png")
      return "Content-Type: image/png";
    else if (content_path == ".jpeg")
      return "Content-Type: image/jpeg";
    else if (content_path == ".gif")
      return "Content-Type: image/gif";
    else if (content_path == ".svg")
      return "Content-Type: image/svg+xml";
    //check if a text file !!
    else if (content_path == ".html")
      return "Content-Type: text/html";
    else if (content_path == ".css")
      return "Content-Type: text/css";
    else if (content_path == ".js")
      return "Content-Type: text/js";
    else if (content_path == ".txt")
      return "Content-Type: text/plain";
    else if (content_path == ".xml")
      return "Content-Type: text/xml";
    //check if a application file !!
    else if (content_path == ".pdf")
      return "Content-Type: application/pdf";
    else if (content_path == ".zip")
      return "Content-Type: application/zip";
    // check if audio file
    else if (content_path == ".mp3")
      return "Content-Type: audio/mpeg";
    else if (content_path == ".wav")
      return "Content-Type: audio/wav";
    // check if video file
    else if (content_path == ".mp4")
      return "Content-Type: video/mp4";
    else if (content_path == ".avi")
      return "Content-Type: video/x-msvideo";
    else if (content_path == ".mov")
      return "Content-Type: video/quicktime";
    // default
    else
      return "Content-Type: application/octet-stream";
  }
  return "Content-Type: text/plain";
}

bool send_file(int client, const std::string& path, const std::string& clean_path,
    const request& req, long long startRequestTime, long long timeout) {

  std::ifstream file(path.c_str());
  if (!file.is_open())
    return false;
  
  //get the full size of the file
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0);
  std::stringstream size_str;
  size_str << size;

  std::string headers =
        "HTTP/1.1 200 OK\r\n"
        + ("Content-Length: " + size_str.str()) + "\r\n"
        + get_the_Content_Type(clean_path) + "\r\n"
        "Connection: close\r\n\r\n";

  send(client, headers.c_str(), headers.size(), 0);
  char buffer[16384]; // 16KB = 16 * 1024 bytes

  while(file.good()) {
    file.read(buffer, sizeof(buffer));
    std::streamsize byte_read = file.gcount(); // get actuale byte read
    size_t bytes_sent = 0;

    while (bytes_sent < (size_t)byte_read)
    {
      ssize_t n = send(client, buffer + bytes_sent, byte_read - bytes_sent, 0);
      if (n <= 0)
        return false;  // client closed or error
      if(time::calcl(startRequestTime, time::clock()) > timeout){
        console.info("Connection timed out while sending file: " + path);
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + error(408).page();
        send(client, response.c_str(), response.length(), 0);
        return false;
      }
      bytes_sent += n;
    }
  }
  console.METHODS(req.getMethod(), req.getPath(), 200, time::calcl(startRequestTime, time::clock()));
  return true;
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

      //check if this route has a GET method allowed
      bool get_allowed = false;
      rt& currentRoute = currentServer.route(i);
      for (size_t i = 0; i < currentRoute.length(); i++)
      {
        if(currentRoute.method(i).c_str() == req.getMethod()){
          get_allowed = true;
          break ;
        }
      }

      if (get_allowed == false)
      {
          std::string response =
              "HTTP/1.1 405 Method Not Allowed\r\n"
              "Content-Type: text/html\r\n\r\n" + error(405).page();

          send(client, response.c_str(), response.size(), 0);
          console.METHODS(req.getMethod(), req.getPath(), 405,
                          time::calcl(startRequestTime, time::clock()));
          return ;
      }
      // check if the requset poinet to forbidden path !
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
      if(is_forbedden_path(sourcePath, part))
        isSecurePath = true;

      if(isSecurePath){
        response = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\n" + error(403).page();
        send(client, response.c_str(), response.length(), 0);
        console.METHODS(req.getMethod(), req.getPath(), 403, time::calcl(startRequestTime, time::clock()));
        return ;
      }
      if (is_dir(fullPath) == true){

        // check if the index is set !!
        if(currentServer.index().empty() == false){
          std::string indexPath = fullPath;
          if (indexPath[indexPath.length() -1] != '/')
            indexPath += "/";
          indexPath += currentServer.index();
          if (send_file(client, indexPath, part, req, startRequestTime, currentServer.timeout()) == true)
            return ;
        }
        // if directory listing is allowed
        if (currentServer.route(i).dictlist() == true){
          // implement directory listing func here ...
        }
        break ;
      }
      if (send_file(client, sourcePath, part, req, startRequestTime, currentServer.timeout()) == true)
        return ;
      break;
    }
    i++;
  }
  send(client, response.c_str(), response.length(), 0);
  console.METHODS(req.getMethod(), req.getPath(), 404, time::calcl(startRequestTime, time::clock()));
}