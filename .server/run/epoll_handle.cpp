#include "epoll_handle.hpp"
#include <server.hpp>
#include <request.hpp>
#include <client.hpp>
#include <sys/socket.h>
#include <response.hpp>
#include <sys/stat.h>
#include "required_checks_epoll.cpp"
#include <run_cgi.hpp>

#define READ_BUFFER_SIZE 4096
std::string methodGet(int client, request& req, ctr& currentServer, long long startRequestTime, Client &clientObj);
std::string methodPost(int client, request& req, ctr& currentServer, long long startRequestTime, Client &clientObj, UserManager &users);
std::string methodDelete(int client, request& req, ctr& currentServer, long long startRequestTime);

// check if we can run CGI !!
int can_start_cgi(request& req, rt& route) {
    // check if the route is allow GET and POST 
    bool method_allowed = false;
    for (size_t i = 0; i < route.length(); i++) {
        if (route.method(i) == req.getMethod()) {
            method_allowed = true;
            break;
        }
    }
    if (!method_allowed)
        return 12; // method has ben allowed
    //check if file exist
    std::string sourcePathToHandle = route.cgiScript();
    struct stat fileStat;
    if (stat(sourcePathToHandle.c_str(), &fileStat) != 0)
        return 14; // file not found
    return 0; // all good
}

int handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd, UserManager &users, std::map<int, int>& cgi_fds, char *envp[]) {
    (void)server_sockets;
    // (void)cgi_fds;
    // Implement the logic to handle read events from the client
    ssize_t byte_readed = read(client, clientObj._buffer_read, sizeof(clientObj._buffer_read) - 1);
    if (byte_readed < 0) {
        return 0; // No data read, try again later (non-blocking socket)
    } else if (byte_readed == 0) {
        close(client);
        return -1; // Client disconnected
    }
    clientObj.read_bytes = static_cast<std::size_t>(byte_readed);
    clientObj._buffer_read[clientObj.read_bytes] = '\0';
    clientObj._request_data.append(clientObj._buffer_read, clientObj.read_bytes);
    check_request_state(clientObj._request_data, clientObj);
    if (clientObj.header_complete == false || clientObj.body_complete == false) {
        return 0; // Wait for more data
    }
    else if (clientObj.header_complete == true && clientObj.body_complete == true) {
        request req(clientObj._request_data);
        long long startRequestTime = time::clock();
        if (req.getBadRequest() != 0) {
            // Handle bad request
            std::map<std::string, std::string> Theaders;
            Theaders["Content-Type"] = "text/html";
            std::string responseStr = response(client, startRequestTime, req.getBadRequest(), Theaders, "", req, currentServer).sendResponse();
            clientObj.response = responseStr;
            // Modify epoll to watch for write events
            ev.events = EPOLLOUT;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
                console.issue("Failed to modify epoll for write event");
                close(client);
                return -1;
            }
            return 0;
        }
        std::string response;
        // check for GCI Here
        rt* route = NULL;
        for (std::size_t i = 0; i < currentServer.length(); i++) {
            if (req.getPath() == currentServer.route(i).path()) {
                route = &currentServer.route(i);
                break;
            }
        }
        if(route && !route->cgiScript().empty()) {
            //check if we can start cgi
            int cgi_check = can_start_cgi(req, *route);
            if(cgi_check == 12) {
                // send error response method not allowed 405
                std::map<std::string, std::string> Theaders;
                Theaders["Content-Type"] = "text/html";
                clientObj.response = ::response(client, startRequestTime, 405, Theaders, "", req, currentServer).sendResponse();
                // switch to write event to send response !
                ev.events = EPOLLOUT;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
                    console.issue("Failed to modify epoll for write event");
                    close(client);
                    return -1;
                }
                return 0;
            }
            if(cgi_check == 14) {
                // send error response file not found 404
                std::map<std::string, std::string> Theaders;
                Theaders["Content-Type"] = "text/html";
                clientObj.response = ::response(client, startRequestTime, 404, Theaders, "", req, currentServer).sendResponse();
                // switch to write event to send response !
                ev.events = EPOLLOUT;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
                    console.issue("Failed to modify epoll for write event");
                    close(client);
                    return -1;
                }
                return 0;
            }
            if(start_cgi(clientObj, req, *route, epoll_fd, cgi_fds, envp) == false) {
                // send error response internal server error 500
                std::map<std::string, std::string> Theaders;
                Theaders["Content-Type"] = "text/html";
                clientObj.response = ::response(client, startRequestTime, 500, Theaders, "", req, currentServer).sendResponse();
                // switch to write event to send response !
                ev.events = EPOLLOUT;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
                    console.issue("Failed to modify epoll for write event");
                    close(client);
                    return -1;
                }
                return 0; // Error starting CGI
            }
            return 0; // CGI started successfully, wait for output
        }
        else if (req.getMethod() == "GET") {
            response = methodGet(client, req, currentServer, startRequestTime, clientObj);
        } else if (req.getMethod() == "POST") {
            response = methodPost(client, req, currentServer, startRequestTime, clientObj, users);
        } else if (req.getMethod() == "DELETE") {
            response = methodDelete(client, req, currentServer, startRequestTime);
        } else {
            response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        }
        clientObj.response = response;
        if (clientObj.response == "") {
            close(client);
            return -1;
        }
        // Modify epoll to watch for write events
        ev.events = EPOLLOUT;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
            console.issue("Failed to modify epoll for write event");
            close(client);
            return -1;
        }
    }

    return 0;
}

// ============================================================================
// HANDLE WRITE EVENT - Non-blocking write to client (supports file streaming)
// ============================================================================
int handle_write_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd) {
    (void)currentServer;
    (void)server_sockets;
    // Implement the logic to handle write events to the client
    if (clientObj.header_sent == false) {
        ssize_t bytes_sent = send(client, clientObj.response.c_str() + clientObj.write_sent, clientObj.response.size() - clientObj.write_sent, 0);
        if (bytes_sent < 0) {
            return 0; // Try again later (non-blocking socket)
        }
        clientObj.write_sent += static_cast<std::size_t>(bytes_sent);
        if (clientObj.write_sent >= clientObj.response.size()) {
            clientObj.header_sent = true;
            // Reset write tracking for file streaming
            clientObj.write_sent = 0;
            clientObj.write_len = 0;
        } else {
            return 0; // Header not fully sent, wait for next write event
        }
    }
    if (clientObj.cgi_complete == true)
    {
        // Send CGI output
        ssize_t bytes_sent = send(client, clientObj.cgi_output.c_str() + clientObj.write_sent, clientObj.cgi_output.size() - clientObj.write_sent, 0);
        if (bytes_sent < 0) {
            return 0; // Try again later (non-blocking socket)
        }
        clientObj.write_sent += static_cast<std::size_t>(bytes_sent);
        if (clientObj.write_sent >= clientObj.cgi_output.size()) {
            clientObj.cgi_complete = false;
            clientObj.reset();
            ev.events = EPOLLIN;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
                console.issue("Failed to modify epoll for read event after CGI");
                close(client);
                return -1;
            }
            return 0;
        } else {
            return 0; // CGI output not fully sent, wait for next write event
        }
    }
    if (clientObj.header_sent == true && clientObj.is_streaming == true)
    {
        // Implement file streaming logic here
        if (clientObj.fd_file != -1)
        {
            // Check if we have buffered data to send first (from partial send)
            if (clientObj.write_len == 0) {
                //read new chunk from file
                fcntl(clientObj.fd_file, F_SETFL, O_NONBLOCK);
                ssize_t byte_to_send = read(clientObj.fd_file, clientObj._buffer_write, sizeof(clientObj._buffer_write));
                if (byte_to_send < 0)
                {
                    console.issue("Failed to read from file for streaming");
                    close(clientObj.fd_file);
                    clientObj.fd_file = -1;
                    close(client);
                    return -1;
                }
                else if (byte_to_send == 0)
                {
                    // End of file - streaming complete
                    clientObj.is_streaming = false;
                    close(clientObj.fd_file);
                    clientObj.fd_file = -1;
                    if (check_for_connection_close(clientObj._request_data) == 1) {
                        close(client);
                        return -1;
                    }
                    clientObj.reset();
                    ev.events = EPOLLIN;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
                        console.issue("Failed to modify epoll for read event after streaming");
                        close(client);
                        return -1;
                    }
                    return 0;
                }
                clientObj.write_len = static_cast<std::size_t>(byte_to_send);
                clientObj.write_sent = 0;
            }

            // Send buffered data
            ssize_t bytes_sent = send(client, clientObj._buffer_write + clientObj.write_sent, 
                                      clientObj.write_len - clientObj.write_sent, 0);
            if (bytes_sent < 0)
            {
                return 0; // Try again later (non-blocking socket)
            }
            
            clientObj.write_sent += static_cast<std::size_t>(bytes_sent);
            clientObj.file_offset += static_cast<std::size_t>(bytes_sent);

            // Check if current buffer fully sent
            if (clientObj.write_sent >= clientObj.write_len) {
                clientObj.write_sent = 0;
                clientObj.write_len = 0;
            }

            // Check if entire file has been sent
            if (clientObj.file_offset >= clientObj.file_size)
            {
                clientObj.is_streaming = false;
                close(clientObj.fd_file);
                clientObj.fd_file = -1;
                if (check_for_connection_close(clientObj._request_data) == 1) {
                    close(client);
                    return -1;
                }
                clientObj.reset();
                ev.events = EPOLLIN;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
                    console.issue("Failed to modify epoll for read event after streaming");
                    close(client);
                    return -1;
                }
            }
            return 0;
        }
    }
    else if (clientObj.header_sent == true && clientObj.is_streaming == false) {
        if (check_for_connection_close(clientObj._request_data) == 1) {
            close(client);
            return -1;
        }
        clientObj.reset();
        ev.events = EPOLLIN;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev) < 0) {
            console.issue("Failed to modify epoll for read event");
            close(client);
            return -1;
        }
    }
    
    return 0;
}