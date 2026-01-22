#include <console.hpp>
#include <server.hpp>
#include <permission.hpp>

Console console;
Server server;
std::vector<std::string> secureFiles;
std::vector<std::string> secureFolders;

void loadPermissions(void) {
  // secureFiles
  secureFiles.push_back(".env");
  secureFiles.push_back(".env.local");
  secureFiles.push_back(".env.production");
  secureFiles.push_back(".env.development");
  secureFiles.push_back("makefile");
  secureFiles.push_back("README.md");
  secureFiles.push_back(".gitignore");
  secureFiles.push_back("robots.txt");
  // secureFolders
  secureFolders.push_back(".git");
  secureFolders.push_back("node_modules");
  secureFolders.push_back(".server");
  secureFolders.push_back(".vscode");
}