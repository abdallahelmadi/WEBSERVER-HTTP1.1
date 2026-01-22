#pragma once 
#include <iostream>
#include <string>
#include <map>
#include <vector>


class Client {
    public:
        int fd;
        char _buffer_read[4096];
        std::size_t read_bytes;
        bool header_complete;
        bool body_complete;
        size_t content_length;
        char _buffer_write[4096];
        bool header_sent;
        int fd_file;
        bool is_streaming;
        std::size_t file_offset;
        std::size_t file_size;
        std::string _request_data;
        std::string response;
        std::size_t write_sent;
        std::size_t write_len;
        int server_index;
        Client() : fd(-1),  header_complete(false), body_complete(false), content_length(0), header_sent(false), fd_file(-1), is_streaming(false), file_offset(0), file_size(0), write_sent(0), write_len(0), server_index(-1) {}
        Client(int socket_fd, int index) : fd(socket_fd), header_complete(false), body_complete(false), content_length(0), header_sent(false), fd_file(-1), is_streaming(false), file_offset(0), file_size(0), write_sent(0), write_len(0), server_index(index) {}
        void reset() {
            _buffer_read[0] = '\0';
            _buffer_write[0] = '\0';
            read_bytes = 0;
            _request_data.clear();
            header_complete = false;
            body_complete = false;
            content_length = 0;
            header_sent = false;
            is_streaming = false;
            file_offset = 0;
            file_size = 0;
            response.clear();
            fd_file = -1;
            // server_index = -1;
            response.clear();
            write_sent = 0;
            write_len = 0;
        }
};