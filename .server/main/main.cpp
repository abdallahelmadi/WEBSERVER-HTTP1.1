#include <main.hpp>
#include <unistd.h>

int main(int ac, char* av[]) {
  clock_tt start = startClock();
  if (ac != 2) {
    console.warning("Run `./webserver --help` for more information");
    return 1;
  } else {
    int r = parseArgument(av[1], start);
    return r;
  }
}