#include <string>
#include <ifaddrs.h>
#include <server.hpp>
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>

std::string getNetworkIP() {
  struct ifaddrs* ifaddr; // linked list of network interfaces <---------------------|
  struct ifaddrs* ifa; // iterator pointer for interfaces (one interface at a time) -|

  if (getifaddrs(&ifaddr) == -1)
    return "0.0.0.0";

  std::string result = "0.0.0.0";
  for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == 0) continue; // skip if no address

    if (ifa->ifa_addr->sa_family == AF_INET) { // check for IPv4 address
      char addressBuffer[INET_ADDRSTRLEN];
      void* addr = &(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr))->sin_addr; // get pointer to IP address
      inet_ntop(AF_INET, addr, addressBuffer, INET_ADDRSTRLEN); // Converts a binary IP address to a human-readable string

      std::string ip = addressBuffer;
      std::string interfaceName = ifa->ifa_name;

      // prefer eth0, otherwise skip loopback
      if (interfaceName == "eth0") {
        result = ip;
        break;
      } else if (ip.substr(0, 3) != "127" && result == "0.0.0.0") {
        result = ip;
      }
    }
  }

  freeifaddrs(ifaddr);
  return result;
}