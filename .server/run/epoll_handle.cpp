#include "epoll_handle.hpp"
#include <server.hpp>
#include <request.hpp>
#include <client.hpp>
#include <sys/socket.h>
#include <type.hpp>
#include <status.hpp>
#include <sstream>

// Forward declarations for method handlers
std::string methodGet(int client, request& req, ctr& currentServer, long long startRequestTime);
std::string methodPost(int client, request& req, ctr& currentServer, long long startRequestTime);
std::string methodDelete(int client, request& req, ctr& currentServer, long long startRequestTime);

// New: Prepare file for streaming (returns headers only, sets up clientObj for streaming)
std::string methodGetStreaming(int client, request& req, ctr& currentServer, long long startRequestTime, Client& clientObj);

// ============================================================================
// CHECK IF HTTP REQUEST IS COMPLETE
// ============================================================================
int is_req_complete(const std::string& request) {
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

// ============================================================================
// PREPARE FILE STREAM - Setup for non-blocking file streaming
// ============================================================================
void prepare_file_stream(Client& clientObj, const std::string& filepath, int status_code, const std::string& content_type) {
    struct stat file_stat;
    if (stat(filepath.c_str(), &file_stat) != 0) {
        return;
    }
    
    // Open file for reading
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) {
        return;
    }
    
    // Set non-blocking
    fcntl(fd, F_SETFL, O_NONBLOCK);
    
    // Setup client for streaming
    clientObj.file_fd = fd;
    clientObj.file_size = file_stat.st_size;
    clientObj.file_sent = 0;
    clientObj.headers_sent = false;
    clientObj.is_streaming_file = true;
    
    // Build HTTP headers
    std::stringstream headers;
    headers << "HTTP/1.1 " << status_code << " " << status(status_code).message() << "\r\n";
    headers << "Content-Type: " << content_type << "\r\n";
    headers << "Content-Length: " << clientObj.file_size << "\r\n";
    headers << "Accept-Ranges: bytes\r\n";
    headers << "Connection: close\r\n";
    headers << "\r\n";
    
    clientObj.response = headers.str();
    clientObj.write_len = clientObj.response.length();
    clientObj.write_sent = 0;
}

// ============================================================================
// HANDLE READ EVENT - Non-blocking read from client
// ============================================================================
int handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd) {
    (void)server_sockets;
    
    // Read data from client (non-blocking)
    ssize_t byte_readed = read(client, clientObj._buffer_read, sizeof(clientObj._buffer_read) - 1);
    
    // Check for errors or connection closed
    if (byte_readed <= 0) {
        // byte_readed == 0 means client closed connection
        // byte_readed < 0 could be EAGAIN (no data) or real error
        if (byte_readed == 0) {
            return -1; // Client closed connection
        }
        // For non-blocking, if no data available, just return and wait
        return 0;
    }

    clientObj._buffer_read[byte_readed] = '\0';
    clientObj._request_data += std::string(clientObj._buffer_read);
    
    // Check if request is complete
    if (is_req_complete(clientObj._request_data)) {
        request req(clientObj._request_data);
        long long startRequestTime = time::clock();
        
        if (req.getMethod() == "GET") {
            // Use streaming version for GET (handles large files)
            clientObj.response = methodGetStreaming(client, req, currentServer, startRequestTime, clientObj);
        } else if (req.getMethod() == "POST") {
            clientObj.response = methodPost(client, req, currentServer, startRequestTime);
        } else if (req.getMethod() == "DELETE") {
            clientObj.response = methodDelete(client, req, currentServer, startRequestTime);
        } else {
            console.issue("Unsupported HTTP method: " + req.getMethod());
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

// ============================================================================
// HANDLE WRITE EVENT - Non-blocking write to client (supports file streaming)
// ============================================================================
int handle_write_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd) {
    (void)currentServer;
    (void)server_sockets;
    
    // First, send HTTP headers if not yet sent
    if (!clientObj.headers_sent) {
        while (clientObj.write_sent < clientObj.write_len) {
            ssize_t bytes_send = send(client, 
                                      clientObj.response.c_str() + clientObj.write_sent, 
                                      clientObj.write_len - clientObj.write_sent, 0);
            
            if (bytes_send < 0) {
                // Would block - try again later
                return 0;
            }
            if (bytes_send == 0) {
                return -1; // Connection closed
            }
            
            clientObj.write_sent += bytes_send;
        }
        clientObj.headers_sent = true;
        
        // If not streaming a file, we're done
        if (!clientObj.is_streaming_file) {
            clientObj.reset();
            ev.data.fd = client;
            ev.events = EPOLLIN;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &ev);
            return 0;
        }
    }
    
    // Stream file content in chunks (non-blocking)
    if (clientObj.is_streaming_file && clientObj.file_fd != -1) {
        while (clientObj.file_sent < clientObj.file_size) {
            // Read chunk from file
            ssize_t to_read = sizeof(clientObj._buffer_write);
            if (clientObj.file_size - clientObj.file_sent < static_cast<std::size_t>(to_read)) {
                to_read = clientObj.file_size - clientObj.file_sent;
            }
            
            ssize_t bytes_read = read(clientObj.file_fd, clientObj._buffer_write, to_read);
            
            if (bytes_read < 0) {
                // Would block or error - try again later
                return 0;
            }
            if (bytes_read == 0) {
                break; // End of file
            }
            
            // Send chunk to client
            ssize_t total_sent = 0;
            while (total_sent < bytes_read) {
                ssize_t bytes_written = send(client, 
                                             clientObj._buffer_write + total_sent, 
                                             bytes_read - total_sent, 0);
                
                if (bytes_written < 0) {
                    // Would block - seek back and try later
                    lseek(clientObj.file_fd, -(bytes_read - total_sent), SEEK_CUR);
                    clientObj.file_sent += total_sent;
                    return 0;
                }
                if (bytes_written == 0) {
                    return -1; // Connection closed
                }
                
                total_sent += bytes_written;
            }
            
            clientObj.file_sent += bytes_read;
        }
        
        // File fully sent
        std::stringstream ss;
        ss << "File streamed: " << clientObj.file_sent << " bytes";
        console.log(ss.str());
        clientObj.reset();
        return -1; // Close connection after file transfer
    }
    
    return 0;
}