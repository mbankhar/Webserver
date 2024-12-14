#pragma once

#include "Server.hpp"
#include "HTTPRequest.hpp"
#include "cgi.hpp"
#include "ServerBlock.hpp"
#include <sys/event.h>
#include <unistd.h>
#include <stdexcept>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <algorithm>
#include <csignal>
#include <atomic>
#include <sstream>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <fstream>
#include <deque>

#define SOCKET_BACKLOG_MAX 10
// #define PORT 8080

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

#define I_AM_THE_BEST true

#ifndef DEBUG
# define DEBUG 0
#endif


#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <map>

using std::string;
using std::cout;
