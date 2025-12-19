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
  for (std::size_t i = 0; i < server.length(); i++) {

    // create socket
    int serverFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFD < 0) {
      console.issue("Failed to create socket for " + server[i].name());
      continue;
    }

    // set socket options
    int opt = 1;
    if (setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
      console.issue("Failed to set socket options for " + server[i].name());
      close(serverFD);
      continue;
    }

    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // bind to all interfaces
    // (for specific IP, add `ntohs(INADDR_LOOPBACK)` or use `inet_addr("x.x.x.x")`)
    address.sin_port = htons(server[i].port()); // bind to specified port

    if (bind(serverFD, (struct sockaddr*)&address, sizeof(address)) < 0) {
      console.issue("Failed to bind socket for " + server[i].name());
      close(serverFD);
      continue;
    }

    // Listen for connections
    if (listen(serverFD, 10) < 0) {
      console.issue("Failed to listen on " + server[i].name());
      close(serverFD);
      continue;
    }

    console.init(server[i].port(), getNetworkIP(), server[i].name(), server[i].version());
    // console.success("Ready in " + (num2str(time::calc(start, time::clock()))) + "ms");

    // Accept and handle requests
    while (true) {
      struct sockaddr_in client_address;
      socklen_t client_len = sizeof(client_address);
      int client_fd = accept(serverFD, (struct sockaddr*)&client_address, &client_len);

      if (client_fd < 0) {
        continue;
      }

      // Read request (simple read)
      char buffer[4096] = {0};
      read(client_fd, buffer, sizeof(buffer) - 1);

      (void)start; // suppress unused variable warning
      std::cout << "Received request:\n" << buffer << std::endl;

      // Send HTTP response
      const char* response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Typpe: application/json\r\n" 
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";

      send(client_fd, response, strlen(response), 0);
      close(client_fd);
    }
  }
  return 1;
}