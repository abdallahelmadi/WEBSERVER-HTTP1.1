#pragma once
#include <sys/epoll.h>
#include <string>
#include <iostream>
#include <server.hpp>
#include <console.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <request.hpp>
#include <cstdlib>
#include <time.hpp>
#include <client.hpp>
#include <sys/stat.h>

class client;

// Size threshold for file streaming (1MB) - files larger than this are streamed
#define FILE_STREAM_THRESHOLD 1048576

int handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd);
int handle_write_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd);
int is_req_complete(const std::string& request);

// New function for preparing file stream response
void prepare_file_stream(Client& clientObj, const std::string& filepath, int status_code, const std::string& content_type);