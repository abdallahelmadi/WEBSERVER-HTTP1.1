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



void handle_read_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj);
void handle_write_event(int client, ctr& currentServer, struct epoll_event& ev, Client& clientObj);
int is_req_complete(const std::string& request);