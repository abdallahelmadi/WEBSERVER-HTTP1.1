#include <console.hpp>
#include <server.hpp>

int main(int ac, char *av[]) {

  (void)ac;
  (void)av;

  std::cout << "Server count: " << server.length() << std::endl;

  server[0].name() = "MyServer";

  return 0;
}