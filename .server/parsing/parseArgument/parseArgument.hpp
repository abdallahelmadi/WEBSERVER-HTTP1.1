#pragma once
#include <clock.hpp>
#include <console.hpp>
#include <fcntl.h>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

int parseArgument(char* argument, clock_tt startClock) throw();

void pathConfig(char* file, clock_tt startClock);
void autoConfig(clock_tt startClock);