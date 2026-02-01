#pragma once
#include <client.hpp>
#include <server.hpp>
#include <request.hpp>


bool    start_cgi(Client& clientObj, request& req, rt& route, int epoll_fd, std::map<int, int>& cgi_fds, char *envp[]);