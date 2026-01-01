#pragma once 
#include <iostream>
#include <string>
#include <map>
#include <vector>


class Client {
    public:
        int fd;
        char _buffer_read[4096];
        char _buffer_write[4096];
        bool _is_writing;
        bool _is_reading;
        std::string _request_data;
        std::string response;
        std::size_t write_sent;
        std::size_t write_len;
        int server_index;
        Client() : fd(-1), _is_writing(false), _is_reading(false), write_sent(0), write_len(0), server_index(-1) {}
        Client(int socket_fd, int index) : fd(socket_fd), _is_writing(false), _is_reading(false), write_sent(0), write_len(0), server_index(index) {}
        void reset() {
            _is_writing = false;
            _is_reading = false;
            _buffer_read[0] = '\0';
            _buffer_write[0] = '\0';
            _request_data.clear();
            response.clear();
            write_sent = 0;
            write_len = 0;
        }
};