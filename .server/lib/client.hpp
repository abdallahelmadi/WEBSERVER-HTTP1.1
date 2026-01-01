#pragma once 
#include <iostream>
#include <string>
#include <map>
#include <vector>


class Client {
    public:
        std::vector<int> _socket_fds;
        int index_fd;
        char _buffer_read[4096];
        char _buffer_write[4096];
        bool _is_writing;
        bool _is_reading;
        std::string _request_data;
        std::string response;
        std::size_t write_sent;
        std::size_t write_len;
        Client() : index_fd(-1), _is_writing(false), _is_reading(false) {}

        void addSocket(int socket_fd) {
            _socket_fds.push_back(socket_fd);
            index_fd++;
        }
        void reset() {
            _is_writing = false;
            _is_reading = false;
            _request_data.clear();
            response.clear();
            write_sent = 0;
            write_len = 0;
        }
};