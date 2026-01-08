#include "epoll_handle.hpp"
#include <server.hpp>
#include <request.hpp>
#include <client.hpp>
#include <sys/socket.h>
#include <response.hpp>


#define READ_BUFFER_SIZE 4096
std::string methodGet(int client, request& req, ctr& currentServer, long long startRequestTime, Client &clientObj);
std::string methodPost(int client, request& req, ctr& currentServer, long long startRequestTime);
std::string methodDelete(int client, request& req, ctr& currentServer, long long startRequestTime);
void check_request_state(const std::string& requestdata, Client& clientObj) {
    //check for header complete
    if (!clientObj.header_complete) {
        std::size_t header_end = clientObj._request_data.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            clientObj.header_complete = true;
            std::size_t content_length_pos = requestdata.find("Content-Length:");
            if (content_length_pos != std::string::npos) {
                std::string length_str = requestdata.substr(content_length_pos + 15, requestdata.find("\r\n", content_length_pos) - (content_length_pos + 15));
                clientObj.content_length = static_cast<size_t>(std::atoi(length_str.c_str()));
            } else {
                clientObj.content_length = 0;
            }
        } else {
            return; // Header not complete yet
        }
    }
    //check for body complete
    if (clientObj.header_complete && !clientObj.body_complete) {
        std::size_t header_end = clientObj._request_data.find("\r\n\r\n");
        std::size_t body_start = header_end + 4;
        if (clientObj._request_data.length() - body_start >= clientObj.content_length) {
            clientObj.body_complete = true;
        }
        else {
            return; // Body not complete yet
        }
    }
}
int check_for_connection_close(const std::string& requestdata) {
    // Implement logic to check for "Connection: close" in the response headers
    
    request req(requestdata);
    std::string connection;
    try {
        connection = req.getHeaders().at("Connection");
    } catch (const std::out_of_range& e) {
        connection = "keep-alive"; // Default to keep-alive if header not present
    }
    if (connection == "close")
    {
        return 1;
    }
    else
    {
        return 0;
    }
    return 0;
}

int handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd) {
    (void)server_sockets;
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
        std::string response;
        if (req.getMethod() == "GET") {
            response = methodGet(client, req, currentServer, startRequestTime, clientObj);
        } else if (req.getMethod() == "POST") {
            response = methodPost(client, req, currentServer, startRequestTime);
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