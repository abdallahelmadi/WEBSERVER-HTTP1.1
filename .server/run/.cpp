#include <server.hpp>
#include <console.hpp>
#include <time.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <request.hpp>

std::string getNetworkIP();
int run(long long start) {

  std::vector<int> sockets; // store server sockets

  std::string networkIP = getNetworkIP(); // get the network IP address

  struct sockaddr_in *serverInfo;
  serverInfo->sin_family = AF_INET; // IPv4
  serverInfo->sin_addr.s_addr = INADDR_ANY; // bind to all interfaces on the device (0.0.0.0)

  for (std::size_t i = 0; i < server.length(); i++) {

    // open socket for each server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      console.issue("Failed to create socket for " + server[i].name());
      continue;
    }

    // setup server info
    serverInfo->sin_port = htons(server[i].port()); // convert to byte order
    if (bind(sockfd, reinterpret_cast<const sockaddr *>(serverInfo), sizeof(struct sockaddr_in)) < 0) {
      console.issue("Failed to bind socket for " + server[i].name());
      close(sockfd);
      continue;
    }

    // set socket options to reuse address
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      console.issue("Failed to set SO_REUSEADDR for " + server[i].name());
      close(sockfd);
      continue;
    }

    // listen for connections from clients (browsers)
    if (listen(sockfd, SOMAXCONN) < 0) {
      console.issue("Failed to listen on socket for " + server[i].name());
      close(sockfd);
      continue;
    }

    // add socket to list
    sockets.push_back(sockfd);






























    // (void)start;

    // request req("GET /hello#?test=b HTTP/1.1\r\n\r\ns");

    // std::cout << "Method: " << req.getMethod() << std::endl;
    // std::cout << "Path: " << req.getPath() << std::endl;
    // std::cout << "HTTP: " << req.getHTTP() << std::endl;
    // std::map<std::string, std::string> headers = req.getHeaders();
    // for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
    //   std::cout << "Header: " << it->first << " => " << it->second << std::endl;
    // }
    // std::cout << "Body: " << req.getBody() << std::endl;

    // if (req.getBadRequest())
    //   console.METHODS(req.getMethod(), req.getPath(), req.getBadRequest(), 1);


  }


  for (std::size_t i = 0; i < sockets.size(); i++) {
    close(sockets[i]); // close all sockets before exiting
  }
  return 0;
}