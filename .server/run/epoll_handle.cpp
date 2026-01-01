#include "epoll_handle.hpp"
#include <server.hpp>
#include <request.hpp>
#include <client.hpp>
#include <sys/socket.h>
std::string methodGet(int client, request& req, ctr& currentServer, long long startRequestTime);
std::string methodPost(int client, request& req, ctr& currentServer, long long startRequestTime);
std::string methodDelete(int client, request& req, ctr& currentServer, long long startRequestTime);
int is_req_complete(const std::string& request) {
    // Implement logic to check if the request is complete
    // For example, check for the presence of a double CRLF indicating the end of headers
    if (request.find("\r\n\r\n") != std::string::npos) {
        std::size_t content_length_pos = request.find("Content-Length:");
        if (content_length_pos != std::string::npos) {
            std::string lenght_str = request.substr(content_length_pos + 15, request.find("\r\n", content_length_pos) - (content_length_pos + 15));
            int content_length = std::atoi(lenght_str.c_str());
            std::size_t body_start = request.find("\r\n\r\n") + 4;
            if (request.length() - body_start < static_cast<std::size_t>(content_length)) {
                return 0; 
            }
        }
        return 1;
    }
    return 0;
}


int handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd) {
    (void)server_sockets;  // Not needed here
    // Implement the logic to handle read events from the client
    // For example, read data from the socket and process it
    ssize_t byte_readed = read(client, clientObj._buffer_read, sizeof(clientObj._buffer_read) - 1);
    if (byte_readed <= 0) {
        // Client disconnected or error - clean up
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client, NULL);
        close(client);
        return -1;  // Signal to remove from clients map
    }
    clientObj._buffer_read[byte_readed] = '\0';
    clientObj._request_data += std::string(clientObj._buffer_read);
    
    if (is_req_complete(clientObj._request_data)) {
        request req(clientObj._request_data);
        std::cout << "Request received:\n" << clientObj._request_data << std::endl;
        long long startRequestTime = time::clock();
        if (req.getMethod() == "GET") {
            clientObj.response = methodGet(client, req, currentServer, startRequestTime);
            std::cout << "Generated GET response."<< clientObj.response << std::endl;
        } else if (req.getMethod() == "POST") {
            clientObj.response = methodPost(client, req, currentServer, startRequestTime);
            std::cout << "Generated POST response." << clientObj.response << std::endl;
        } else if (req.getMethod() == "DELETE") {
            clientObj.response = methodDelete(client, req, currentServer, startRequestTime);
            std::cout << "Generated DELETE response." << clientObj.response << std::endl;
        } else {
            console.issue("Unsupported HTTP method: " + req.getMethod());
            // Send 405 Method Not Allowed
            clientObj.response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
        }
        clientObj.write_len = clientObj.response.length();
        clientObj.write_sent = 0;
        
        // Switch to EPOLLOUT to send response
        ev.data.fd = client;
        ev.events = EPOLLOUT;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev);
    }
    return 0;
}
int handle_write_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd) {
    (void)currentServer;
    (void)server_sockets;
    
    // Send remaining response data
    const char* data = clientObj.response.c_str() + clientObj.write_sent;
    std::cout << "data to send:\n" << data << std::endl;
    std::size_t remaining = clientObj.write_len - clientObj.write_sent;
    
    ssize_t bytes_sent = send(client, data, remaining, 0);
    if (bytes_sent < 0) {
        console.issue("Error sending response to client");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client, NULL);
        close(client);
        return -1;  // Signal to remove from clients map
    }
    
    clientObj.write_sent += bytes_sent;
    
    if (clientObj.write_sent < clientObj.write_len) {
        return 0;  // Wait for next EPOLLOUT
    }
    
    // Response fully sent - check for keep-alive
    bool keep_alive = (clientObj._request_data.find("Connection: keep-alive") != std::string::npos) ||
                      (clientObj._request_data.find("HTTP/1.1") != std::string::npos && 
                       clientObj._request_data.find("Connection: close") == std::string::npos);
    
    if (keep_alive) {
        // Reset for next request
        clientObj.reset();
        ev.data.fd = client;
        ev.events = EPOLLIN;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev);
        return 0;
    } else {
        // Close connection
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client, NULL);
        close(client);
        return -1;  // Signal to remove from clients map
    }
}
