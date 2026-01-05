#include "epoll_handle.hpp"
#include <server.hpp>
#include <request.hpp>
#include <client.hpp>
#include <sys/socket.h>
#include <cerrno>
std::string methodGet(int client, request& req, ctr& currentServer, long long startRequestTime);
std::string methodPost(int client, request& req, ctr& currentServer, long long startRequestTime);
std::string methodDelete(int client, request& req, ctr& currentServer, long long startRequestTime);
int is_req_complete(const std::string& request) {
    // Implement logic to check if the request is complete
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
    (void)server_sockets;
    // Implement the logic to handle read events from the client
    ssize_t byte_readed = read(client, clientObj._buffer_read, sizeof(clientObj._buffer_read) - 1);

    clientObj._buffer_read[byte_readed] = '\0';
    clientObj._request_data += std::string(clientObj._buffer_read);
    
    if (is_req_complete(clientObj._request_data)) {
        request req(clientObj._request_data);
        long long startRequestTime = time::clock();
        if (req.getMethod() == "GET") {
            clientObj.response = methodGet(client, req, currentServer, startRequestTime);
        } else if (req.getMethod() == "POST") {
            clientObj.response = methodPost(client, req, currentServer, startRequestTime);
        } else if (req.getMethod() == "DELETE") {
            clientObj.response = methodDelete(client, req, currentServer, startRequestTime);
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
    // Implement the logic to handle write events to the client
    ssize_t bytes_send = send(client, clientObj.response.c_str() + clientObj.write_sent, clientObj.write_len - clientObj.write_sent, 0);
    if (bytes_send < 0) {
        console.issue("Failed to send response to client");
        return -1;
    }
    clientObj.write_sent += bytes_send;
    if (clientObj.write_sent >= clientObj.write_len) {
        // Response fully sent, switch back to EPOLLIN to read new requests
        clientObj.reset();
        ev.data.fd = client;
        ev.events = EPOLLIN;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev);
    }
    else
    {
        // More data to send, keep EPOLLOUT
        ev.data.fd = client;
        ev.events = EPOLLOUT;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev);
    }
    return 0;
}