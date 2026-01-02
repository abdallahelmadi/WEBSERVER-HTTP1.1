#pragma once 
#include <iostream>
#include <string>
#include <map>
#include <vector>

// ============================================================================
// CLIENT STATE - Tracks each connected client for non-blocking I/O
// Supports chunked file streaming for large files (videos, etc.)
// ============================================================================

class Client {
    public:
        int fd;                         // Client socket file descriptor
        char _buffer_read[4096];        // Buffer for reading requests
        char _buffer_write[65536];      // Buffer for writing responses (64KB chunks)
        bool _is_writing;               // Currently in writing mode?
        bool _is_reading;               // Currently in reading mode?
        std::string _request_data;      // Accumulated request data
        std::string response;           // Response headers + small body
        std::size_t write_sent;         // Bytes of response already sent
        std::size_t write_len;          // Total response length
        int server_index;               // Which server config this client belongs to
        
        // === FILE STREAMING FIELDS (for large files like videos) ===
        int file_fd;                    // File descriptor for streaming (-1 if none)
        std::size_t file_size;          // Total file size
        std::size_t file_sent;          // Bytes of file already sent
        bool headers_sent;              // Have HTTP headers been sent?
        bool is_streaming_file;         // Are we streaming a file?
        
        Client() : fd(-1), _is_writing(false), _is_reading(false), 
                   write_sent(0), write_len(0), server_index(-1),
                   file_fd(-1), file_size(0), file_sent(0), 
                   headers_sent(false), is_streaming_file(false) {}
        
        Client(int socket_fd, int index) : fd(socket_fd), _is_writing(false), _is_reading(false), 
                                           write_sent(0), write_len(0), server_index(index),
                                           file_fd(-1), file_size(0), file_sent(0),
                                           headers_sent(false), is_streaming_file(false) {}
        
        void reset() {
            _is_writing = false;
            _is_reading = false;
            _buffer_read[0] = '\0';
            _buffer_write[0] = '\0';
            _request_data.clear();
            response.clear();
            write_sent = 0;
            write_len = 0;
            // Close file if streaming
            if (file_fd != -1) {
                close(file_fd);
                file_fd = -1;
            }
            file_size = 0;
            file_sent = 0;
            headers_sent = false;
            is_streaming_file = false;
        }
};