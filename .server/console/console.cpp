#include <console.hpp>

Console console; /* actual definition */

void Console::init(
  int port,
  std::string network,
  std::string name,
  std::string version
) const throw() {
  std::cout
  << std::endl
  << "> "
  << name
  << "@"
  << version
  << " dev"
  << std::endl
  << "> webserver dev -p "
  << port
  << std::endl
  << std::endl
  << "\033[1;38;2;43;14;68m   ▲ Webserver 1.0.0\033[0m"
  << std::endl
  << "   - Local:   http://localhost:"
  << port
  << std::endl
  << "   - Network: http://"
  << network
  << ":"
  << port
  << std::endl
  << std::endl
  << " \033[38;2;76;175;80m✓\033[0m Starting..."
  << std::endl;
}

void Console::success(char const* s) const throw() {
  std::cout << " \033[38;2;76;175;80m✓\033[0m " << s << std::endl;
}

void Console::issue(char const* i) const throw() {
  std::cout << " \033[38;2;255;82;82m✗\033[0m " << i << std::endl;
}

void Console::info(char const* i) const throw() {
  std::cout << " \033[38;2;66;165;245m•\033[0m " << i << std::endl;
}

void Console::warning(char const* w) const throw() {
  std::cout << " \033[38;2;255;165;0m•\033[0m " << w << std::endl;
}

void Console::log(char const* l) const throw() {
  std::cout << " " << l << std::endl;
}

void Console::GET(char const* path, int status, int in) const throw() {
  std::string statusColor = "\033[0m";
  if (status >= 200 && status < 300)
    statusColor = "\033[38;2;76;175;80m";
  else if (status >= 300 && status < 400)
    statusColor = "\033[38;2;66;165;245m";
  else if (status >= 400 && status < 500)
    statusColor = "\033[38;2;255;165;0m";
  else if (status >= 500 && status < 600)
    statusColor = "\033[38;2;255;82;82m";
  std::cout
  << " GET "
  << path
  << " "
  << statusColor
  << status
  << "\033[0m"
  << " in "
  << in
  << "ms"
  << std::endl;
}

void Console::POST(char const* path, int status, int in) const throw() {
  std::string statusColor = "\033[0m";
  if (status >= 200 && status < 300)
    statusColor = "\033[38;2;76;175;80m";
  else if (status >= 300 && status < 400)
    statusColor = "\033[38;2;66;165;245m";
  else if (status >= 400 && status < 500)
    statusColor = "\033[38;2;255;165;0m";
  else if (status >= 500 && status < 600)
    statusColor = "\033[38;2;255;82;82m";
  std::cout
  << " POST "
  << path
  << " "
  << statusColor
  << status
  << "\033[0m"
  << " in "
  << in
  << "ms"
  << std::endl;
}

void Console::DELETE(char const* path, int status, int in) const throw() {
  std::string statusColor = "\033[0m";
  if (status >= 200 && status < 300)
    statusColor = "\033[38;2;76;175;80m";
  else if (status >= 300 && status < 400)
    statusColor = "\033[38;2;66;165;245m";
  else if (status >= 400 && status < 500)
    statusColor = "\033[38;2;255;165;0m";
  else if (status >= 500 && status < 600)
    statusColor = "\033[38;2;255;82;82m";
  std::cout
  << " DELETE "
  << path
  << " "
  << statusColor
  << status
  << "\033[0m"
  << " in "
  << in
  << "ms"
  << std::endl;
}