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
#include <users.hpp>
#include <client.hpp>
#include <sys/stat.h>

class client;

int handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd, UserManager &users, std::map<int, int>& cgi_fds, char *envp[]);
int handle_write_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj, std::vector<int>& server_sockets, int epoll_fd);