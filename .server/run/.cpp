#include <server.hpp>
#include <console.hpp>
#include <time.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sstream>
#include <poll.h>
#include <request.hpp>
#include <permission.hpp>
#include <fstream>
#include <status.hpp>
#include <error.hpp>
#include <signal.h>
#include <cstdlib>
#include <ctime> // for std::time

std::string getNetworkIP();
void methodGet(int client, request& req, ctr& currentServer, long long startRequestTime);
void methodPost(int client, request& req, ctr& currentServer, long long startRequestTime);
void methodDelete(int client, request& req, ctr& currentServer, long long startRequestTime);
int run(long long start) {

  // Ignore SIGPIPE to prevent crash when client disconnects
  signal(SIGPIPE, SIG_IGN);
  std::srand(std::time(NULL));

  std::vector<struct pollfd> pollfds; // list of poll file descriptors
  std::string networkIP = getNetworkIP(); // get the network IP address

  struct sockaddr_in serverInfo;
  serverInfo.sin_family = AF_INET; // IPv4
  serverInfo.sin_addr.s_addr = INADDR_ANY; // bind to all interfaces on the device (0.0.0.0)

  for (std::size_t i = 0; i < server.length(); i++) {

    // open socket for each server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      console.issue("Failed to create socket for " + server[i].name());
      continue;
    }

    // set socket options to reuse address
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      console.issue("Failed to set SO_REUSEADDR for " + server[i].name());
      close(sockfd);
      continue;
    }

    // setup server info (i want to listen)
    // bind(): server side, connect(): client side
    serverInfo.sin_port = htons(server[i].port()); // convert to byte order
    if (bind(sockfd, reinterpret_cast<const sockaddr*>(&serverInfo), sizeof(struct sockaddr_in)) < 0) {
      console.issue("Failed to bind socket for " + server[i].name());
      close(sockfd);
      continue;
    }

    // create kernel queue (10)
    // The OS kernel handles incoming connections automatically and stores them in the queue
    if (listen(sockfd, 10) < 0) {
      console.issue("Failed to listen on socket for " + server[i].name());
      close(sockfd);
      continue;
    }

    // add socket to pollfds list
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN; // event: watch for incoming data/connections
    pollfds.push_back(pfd);

    // log server start
    console.init(server[i].port(), networkIP, server[i].name(), server[i].version());

  }

  console.success("Ready in " + time::calcs(start, time::clock()) + "ms\n");

  while (true) {

    // poll tells you which server socket has a connection READY to accept
    // the browser close connection after get response (!keep-alive)
    if (poll(pollfds.data(), pollfds.size(), -1) < 0) {
      console.issue("poll() failed");
      continue;
    }

    // check which sockets are ready to get accepted and server data
    for (size_t i = 0; i < pollfds.size(); i++) {
      if (pollfds[i].revents & POLLIN) { /* check if this even pollfds[i].revents = POLLIN
                                            mean: what's the socket get event === POLLIN, because poll detected it */
        int client = accept(pollfds[i].fd, NULL, NULL); // this params (NULL) returns information about user (IP,..)
        if (client < 0) continue;

        char requestBuffer[server[i].bodylimit() + 2048]; // buffer to store request

        if (read(client, requestBuffer, sizeof(requestBuffer)) < 0) { // store request
          console.issue("Failed to read from client");
          close(client);
          continue;
        }

        long long startRequestTime = time::clock();

        request req(requestBuffer); // parse request

        if (req.getBadRequest()) {
          std::stringstream buffer;
          buffer << req.getBadRequest();
          std::string badReqStr = buffer.str();

          // check if user create page error for this bad request code
          std::ifstream file;
          std::string customErrorPagePath = server[i].errorPages()[badReqStr];
          if (!customErrorPagePath.empty()) {
            file.open(customErrorPagePath.c_str());
          }
          if (file.is_open()) {
            std::stringstream body;
            body << file.rdbuf();
            file.close();
            std::stringstream response;
            response << "HTTP/1.1 " << req.getBadRequest() << " " << status(req.getBadRequest()).message() << "\r\n\r\n" << body.str();
            std::string responseStr = response.str();
            send(client, responseStr.c_str(), responseStr.length(), 0);
            console.METHODS(req.getMethod(), req.getPath(), req.getBadRequest(), time::calcl(startRequestTime, time::clock()));
            close(client);
            continue;
          }
          file.close();

          // send default error page
          std::string body = error(req.getBadRequest()).page();
          std::stringstream response;
          response << "HTTP/1.1 " << req.getBadRequest() << " " << status(req.getBadRequest()).message() << "\r\n\r\n" << body;
          std::string responseStr = response.str();
          send(client, responseStr.c_str(), responseStr.length(), 0);
          console.METHODS(req.getMethod(), req.getPath(), req.getBadRequest(), time::calcl(startRequestTime, time::clock()));
          close(client);
          continue;
        }

        if (req.getMethod() == "GET") {
          methodGet(client, req, server[i], startRequestTime);
        } else if (req.getMethod() == "POST") {
          methodPost(client, req, server[i], startRequestTime);
        } else if (req.getMethod() == "DELETE") {
          methodDelete(client, req, server[i], startRequestTime);
        }

        close(client);
      }
    }

  }

  return 0;
}