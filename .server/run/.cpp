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
#include <sys/epoll.h>
#include "epoll_handle.hpp"
#include <client.hpp>
#include <algorithm>
#include <fcntl.h>
#include <users.hpp>
#include <ctime> // for std::time
#include <response.hpp>
#include <sys/wait.h> // track cgi exit status

std::string getNetworkIP();
int run(long long start) {

  std::map<int, Client> clients;
  std::vector<int> server_sockets;
  // Ignore SIGPIPE to prevent crash when client disconnects
  signal(SIGPIPE, SIG_IGN);
  std::map<int, int> cgi_fds;
  std::srand(std::time(NULL));
  UserManager Users;

  std::vector<struct pollfd> pollfds; // list of poll file descriptors
  std::string networkIP = getNetworkIP(); // get the network IP address

  struct sockaddr_in serverInfo;
  serverInfo.sin_family = AF_INET; // IPv4
  serverInfo.sin_addr.s_addr = INADDR_ANY; // bind to all interfaces on the device (0.0.0.0)

  // struct epoll_event event;
  // struct epoll_event ev;
  int epollfd = epoll_create1(0);
  if (epollfd < 0) {
    console.issue("Failed to create epoll file descriptor");
    return -1;
  }
  // Client clientObj;
  for (std::size_t i = 0; i < server.length(); i++) {

    // open socket for each server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      console.issue("Failed to create socket for " + server[i].name());
      continue;
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK); // set non-blocking

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
    server_sockets.push_back(sockfd);
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    // log server start
    console.init(server[i].port(), networkIP, server[i].name(), server[i].version());
  }
  console.success("Ready in " + time::calcs(start, time::clock()) + "ms\n");
  while (true)
  {
    struct epoll_event event[1000];
    int event_count = epoll_wait(epollfd, event, 1000, 1000); // 1 second timeout
  // ========== CHECK TIMEOUTS HERE (OUTSIDE event loop) ==========
    long long current_time = time::clock();
    for (std::map<int, int>::iterator it = cgi_fds.begin(); it != cgi_fds.end(); ) {
      int cgi_fd = it->first;
      int client_fd = it->second;
      
      std::map<int, Client>::iterator client_it = clients.find(client_fd);
      if (client_it == clients.end()) { // Client not found, clean up
        epoll_ctl(epollfd, EPOLL_CTL_DEL, cgi_fd, NULL);
        close(cgi_fd);
        cgi_fds.erase(it++);  // <-- Post-increment: erase current, move to next
        continue;
      }
      
      Client& cl = client_it->second;
      // std::cout << "CGI check: elapsed=" << (current_time - cl.cgi_start_time) 
      //           << "ms, timeout=" << cl.cgi_timeout_ms << "ms" << std::endl;
      
      if (cl.cgi_timeout_ms > 0 && (current_time - cl.cgi_start_time) > cl.cgi_timeout_ms) {
        if (cl.cgi_pid > 0) {
          kill(cl.cgi_pid, SIGKILL);
        }
        close(cgi_fd);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, cgi_fd, NULL);
        
        // Build timeout response with correct Content-Length
        std::map<std::string, std::string> Theaders;
        Theaders["Content-Type"] = "text/html";
        cl.response = response(client_fd, cl.cgi_start_time, 504, Theaders, "", request(cl._request_data), ctr()).sendResponse();
        cl.cgi_complete = false;  // Don't use CGI output path
        cl.cgi_output.clear();
        cl.header_sent = false;   // Reset so header gets sent
        cl.write_sent = 0;
        
        struct epoll_event ev;
        ev.data.fd = client_fd;
        ev.events = EPOLLOUT;
        epoll_ctl(epollfd, EPOLL_CTL_MOD, client_fd, &ev); // Switch to write event
        epoll_ctl(epollfd, EPOLL_CTL_DEL, cgi_fd, NULL); // Remove CGI fd from epoll
        cgi_fds.erase(it++);
        continue;
      }
      ++it;
    }

    for (int i = 0; i < event_count; i++)
    {
      int fd_check = event[i].data.fd;
      // Find if this is a server socket and get its index
      std::vector<int>::iterator it = std::find(server_sockets.begin(), server_sockets.end(), fd_check);
      if (it != server_sockets.end())
      {
        // This is a server socket - accept new connection
        int server_idx = it - server_sockets.begin();
        int client = accept(fd_check, NULL, NULL);  // Use fd_check, not ev.data.fd
        if (client < 0) continue;
        fcntl(client, F_SETFL, O_NONBLOCK); // set non-blocking
        struct epoll_event ev;
        ev.data.fd = client;
        ev.events = EPOLLIN;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &ev);
        clients[client] = Client(client, server_idx);
      }
      else
      {
        if (cgi_fds.find(fd_check) != cgi_fds.end())
        {
            int client_fd = cgi_fds[fd_check];
            Client &cl = clients[client_fd];
            char buffer[4096];
            ssize_t bytes_read = read(fd_check, buffer, sizeof(buffer) - 1); // leave space for null terminator
            buffer[bytes_read] = '\0'; // null-terminate the string
            
            if(bytes_read > 0){
              // std::cout << "pipe is ready to read " << bytes_read << " bytes" << std::endl;
              // std::cout.write(buffer, bytes_read);
              cl.cgi_output.append(buffer, bytes_read);
            }
            if(bytes_read == 0){
              // std::cout << "pipe is ready to read " << bytes_read << " bytes" << std::endl;
              // std::cout.write(buffer, bytes_read);
              // buffer[bytes_read] = '\0';
              // cl.cgi_output.append(buffer, bytes_read);

              //check exit status
              int status;
              waitpid(cl.cgi_pid, &status, 0);
              // check if exited normally
              int exit_code = WEXITSTATUS(status);
              if (exit_code != 0){
                  // handle error response
                  cl.cgi_complete = true;
                  std::map<std::string, std::string> Theaders;
                  Theaders["Content-Type"] = "text/html";
                  cl.response = ::response(client_fd, cl.cgi_start_time, 500, Theaders, "", request(cl._request_data), ctr()).sendResponse();
                  struct epoll_event ev;
                  ev.data.fd = client_fd;
                  ev.events = EPOLLOUT; // Ready to write response
                  epoll_ctl(epollfd, EPOLL_CTL_MOD, client_fd, &ev); // Switch to write event
                  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd_check, NULL); // Remove CGI fd from epoll
                  cgi_fds.erase(fd_check); // Remove from tracking map
              }
              else{

                  // befor build response make sure if the ouptut of cgi has heders or not
                  size_t header_end = cl.cgi_output.find("\r\n\r\n"); // look for end of headers
                  std::string headers;
                  // check for header parsing errors to send 502 Bad Gateway
                  bool has_errors = false;
                  bool header_found = false;
                  std::map<std::string, std::string> cgi_headers;
                  if (header_end != std::string::npos) {
                    header_found = true;
                    // std::cout << "CGI output contains headers." << std::endl;
                    headers = cl.cgi_output.substr(0, header_end + 4); // extract headers with \r\n\r\n
                    cl.cgi_output = cl.cgi_output.substr(header_end + 4); // skip past \r\n\r\n
                    //parse headers
                    std::istringstream header_stream(headers); // create stream from headers string
                    std::string line;
                    
                    std::string CRLF = "\r\n";

                    while (std::getline(header_stream, line)) {

                        // Handle CRLF: remove trailing '\r'
                        if (!line.empty() && line[line.size() - 1] == '\r') {
                            line.erase(line.size() - 1);
                        }

                        // Empty line = end of headers
                        if (line.empty()) {
                            break;
                        }

                        size_t colon_pos = line.find(':');
                        if (colon_pos == std::string::npos) {
                            std::cout << "Invalid header: missing ':' in line: " << line << std::endl;
                            has_errors = true;
                            break;
                        }

                        std::string key = line.substr(0, colon_pos);
                        std::string value = line.substr(colon_pos + 1);

                        // Key must not be empty
                        if (key.empty()) {
                            std::cout << "Invalid header: empty key in line: " << line << std::endl;
                            has_errors = true;
                            break;
                        }
                        
                        // Key must be alphanumeric or hyphens only
                        for (std::string::size_type i = 0; i < key.size(); i++) {
                            char c = key[i];
                            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') {
                                std::cout << "Invalid header: bad character in key in line: " << line << std::endl;
                                has_errors = true;
                                break;
                            }
                        }
                        if (has_errors) break;

                        // Value must not be empty
                        if (value.empty()) {
                            std::cout << "Invalid header: empty value in line: " << line << std::endl;
                            has_errors = true;
                            break;
                        }

                        // OPTIONAL strict check: require space after colon
                        /*
                        if (value[0] != ' ') {
                            std::cout << "Invalid header: missing space after ':' in line: " << line << std::endl;
                            has_errors = true;
                            break;
                        }
                        */

                        cgi_headers[key] = value;
                    }

                    if (has_errors) {
                      // handle error response for header parsing errors
                      cl.cgi_complete = true;
                      std::map<std::string, std::string> Theaders;
                      Theaders["Content-Type"] = "text/html";
                      cl.response = ::response(client_fd, cl.cgi_start_time, 502, Theaders, "", request(cl._request_data), ctr()).sendResponse();
                      struct epoll_event ev;
                      ev.data.fd = client_fd;
                      ev.events = EPOLLOUT; // Ready to write response
                      epoll_ctl(epollfd, EPOLL_CTL_MOD, client_fd, &ev); // Switch to write event
                      epoll_ctl(epollfd, EPOLL_CTL_DEL, fd_check, NULL); // Remove CGI fd from epoll
                      cgi_fds.erase(fd_check); // Remove from tracking map
                      continue; // Move to next event
                    }
                  }
                  // check if no headers send response normally
                  if(!header_found){
                    cl.cgi_complete = true;
                    std::stringstream response_cgi;
                    response_cgi << "HTTP/1.1 200 OK\r\n";
                    response_cgi << "Content-Length: " << cl.cgi_output.size() << "\r\n";
                    response_cgi << "Content-Type: text/html\r\n";
                    response_cgi << "\r\n";
                    cl.response = response_cgi.str(); 
                    struct epoll_event ev;
                    ev.data.fd = client_fd;
                    ev.events = EPOLLOUT; // Ready to write response
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, client_fd, &ev); // Switch to write event
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd_check, NULL); // Remove CGI fd from epoll
                    cgi_fds.erase(fd_check); // Remove from tracking map
                    request ee(cl._request_data); // parse request again for logging
                    console.METHODS(ee.getMethod(), ee.getPath(), 200, cl.time);
                  }
                  // if headers found and no errors build response !!
                  else {
                    cl.cgi_complete = true;
                    // check if we have the important headers (Content-Type, Content-Length, Status)
                    std::stringstream headers_stream;
                    if(headers.find("Content-Type") != std::string::npos){
                      headers_stream << "Content-Type: " << cgi_headers["Content-Type"] << "\r\n";
                    } else {
                      headers_stream << "Content-Type: text/html\r\n";
                    }
                    if(headers.find("Content-Length") != std::string::npos){
                      headers_stream << "Content-Length: " << cgi_headers["Content-Length"] << "\r\n";
                    } else {
                      headers_stream << "Content-Length: " << cl.cgi_output.size() << "\r\n";
                    }
                    // check if we have status header later
                    // add other headers from cgi output without duplicates
                    for (std::map<std::string, std::string>::iterator it = cgi_headers.begin(); it != cgi_headers.end(); ++it) {
                      if (it->first != "Content-Type" && it->first != "Content-Length") {
                        headers_stream << it->first << ": " << it->second << "\r\n";
                      }
                    }
                    headers_stream << "\r\n"; // end of headers
                    // build final response
                    cl.response = "HTTP/1.1 200 OK\r\n" + headers_stream.str() + cl.cgi_output;
                    struct epoll_event ev;
                    ev.data.fd = client_fd;
                    ev.events = EPOLLOUT; // Ready to write response
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, client_fd, &ev); // Switch to write event
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd_check, NULL); // Remove CGI fd from epoll
                    cgi_fds.erase(fd_check); // Remove from tracking map
                    request ee(cl._request_data); // parse request again for logging
                    console.METHODS(ee.getMethod(), ee.getPath(), 200, cl.time);
                  }
              }
            }
            continue; // Move to next event
        }
        // This is a client socket
        std::map< int, Client >::iterator client_it = clients.find(fd_check);
        if (client_it == clients.end()) {
          epoll_ctl(epollfd, EPOLL_CTL_DEL, fd_check, NULL);
          close(fd_check);
          continue;
        }
        // int client_fd = event[i].data.fd;
        Client& clientObj = client_it->second;
        int server_idx = clientObj.server_index;
        
        if (event[i].events & (EPOLLERR | EPOLLHUP))
        {
          // Client disconnected or error
          epoll_ctl(epollfd, EPOLL_CTL_DEL, fd_check, NULL);
          close(fd_check);
          clients.erase(fd_check);
        }       
        else if (event[i].events & EPOLLIN)
        {
          if (handle_read_event(fd_check, server[server_idx], event[i], clientObj, server_sockets, epollfd, Users, cgi_fds) < 0) {
            epoll_ctl(epollfd, EPOLL_CTL_DEL, fd_check, NULL);
            close(fd_check);
            clients.erase(fd_check);
          }
        }
        else if (event[i].events & EPOLLOUT)
        {
          if (handle_write_event(fd_check, server[server_idx], event[i], clientObj, server_sockets, epollfd) < 0) {
            epoll_ctl(epollfd, EPOLL_CTL_DEL, fd_check, NULL);
            close(fd_check);
            clients.erase(fd_check);
          }
        }
      }
    }

  }
  return 0;
}