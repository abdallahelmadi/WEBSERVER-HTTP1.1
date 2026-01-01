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


void handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj) {
    // Implement the logic to handle read events from the client
    // For example, read data from the socket and process it
    clientObj.addSocket(client);
    ssize_t byte_readed = read(client, clientObj._buffer_read, sizeof(clientObj._buffer_read) - 1);
    if (byte_readed < 0) {
        console.issue("Error reading from client socket");
        return;
    }
    clientObj._buffer_read[byte_readed] = '\0';
    if (is_req_complete(std::string(clientObj._buffer_read))) {
        request req(std::string(clientObj._buffer_read));
        clientObj._request_data = std::string(clientObj._buffer_read);
        long long startRequestTime = time::clock();
        if (req.getMethod() == "GET") {
            clientObj.response = methodGet(client, req, currentServer, startRequestTime);
            clientObj.write_len = clientObj.response.length();
            clientObj.write_sent = 0;
        } else if (req.getMethod() == "POST") {
            clientObj.response = methodPost(client, req, currentServer, startRequestTime);
            clientObj.write_len = clientObj.response.length();
            clientObj.write_sent = 0;
        } else if (req.getMethod() == "DELETE") {
            clientObj.response = methodDelete(client, req, currentServer, startRequestTime);
            clientObj.write_len = clientObj.response.length();
            clientObj.write_sent = 0;
        } else {
            console.issue("Unsupported HTTP method");
            return;
        }
        ev.data.fd = clientObj._socket_fds[clientObj.index_fd];
        ev.events = EPOLLOUT;
    }

}
void handle_write_event(int client , ctr& currentServer, struct epoll_event& ev, Client& clientObj) {
    // Implement the logic to handle write events to the client
    // For example, send data to the socket
    //     send remaining response
    
    // if send error
    //     close client
    
    // update write_sent
    
    // if write_sent < write_len
    //     return  // wait for next EPOLLOUT
    
    // // response fully sent
    // if keep_alive
    //     reset buffers
    //     epoll_ctl MOD → EPOLLIN
    // else
    //     close client
    (void)currentServer;
    if (ev.data.fd == clientObj._socket_fds[clientObj.index_fd]) {
        ssize_t bytes_sent = send(client, clientObj.response.c_str(), clientObj.response.length(), 0);
        if (bytes_sent < 0) {
            console.issue("Error sending response to client");
            return;
        }
        clientObj.write_sent += bytes_sent;
        if (clientObj.write_sent < clientObj.write_len) {
            return;  // wait for next EPOLLOUT
        }
        // response fully sent
        if (clientObj.write_sent == clientObj.write_len) {
            std::string connection_header = "Connection: close"; // Default to close
            if (clientObj._request_data.find(connection_header) != std::string::npos) {
                // close connection
                close(client);
            } else {
                // reset buffers for keep-alive
                clientObj.reset();
                ev.data.fd = clientObj._socket_fds[clientObj.index_fd];
                ev.events = EPOLLIN;
            }
        }
        
    }
}
