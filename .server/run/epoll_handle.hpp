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
class client;

int handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd);
int handle_write_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd);
int is_req_complete(const std::string& request);