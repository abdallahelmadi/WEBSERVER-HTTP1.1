#include <server.hpp>
#include <console.hpp>
#include <fstream>
#include <sstream>

static int help(void) {
  console.info("Usage: ./webserver [options]");
  console.info("Options:");
  console.log("  --help                     Show this help message");
  console.log("  --config-rules             Show all available configuration rules");
  console.log("  --config-file=<filename>   Specify a custom configuration file, this file");
  console.log("                             must be in JSON format and in .server/.config/ folder");
  console.log("  --auto-config              Generate a default configuration file in (for static pages only)");
  return 2;
}

static int configRules(void) {
  console.info("Read .server/.config/example.json for all available configuration rules");
  console.success("servers: An array of server configurations");
  console.log("         │");
  console.log("         ├───── port:        The port number the server will listen on (optional, default: 3000)");
  console.log("         ├───── name:        The name of your web application, for each web application");
  console.log("         │                   should be unique and inside /app folder in the root");
  console.log("         │                   directory of /.server (required)");
  console.log("         ├───── version:     The version of your web application (optional, default: 0.1.0)");
  console.log("         ├───── notfound:    Path to the HTML file to serve for 404 Not Found errors (optional, default)");
  console.log("         ├───── servererror: Path to the HTML file to serve for 500 Internal Server errors (optional, default)");
  console.log("         ├───── log:         Name of the access log file `.server/.log/` (optional, default: <name>.log)");
  console.log("         ├───── bodylimit:   Maximum size of the request body in bytes (optional, default: 1048576)");
  console.log("         ├───── timeout:     Request timeout in milliseconds (optional, default: 30000)");
  console.log("         ├───── uploaddir:   Directory to store uploaded files (optional, default: app/<name>/uploads)");
  console.log("         ├───── index:       Default file to serve when accessing a directory (optional, default: index.html)");
  console.log("         ├───── root:        Root directory for serving static files (optional, default: /app/<name>/)");
  console.log("         └───── routes:      An array of route configurations (optional)");
  console.log("                       |");
  console.log("                       ├───── path:   The URL path for the route (required)");
  console.log("                       ├───── source: The file to serve for this route (optional, none)");
  console.log("                       ├───── dictlist: A boolean to enable directory listing (optional, default: false)");
  console.log("                       ├───── redirect: The URL to redirect to (optional, none)");
  console.log("                       ├───── cgi_script:      The CGI script to execute for this route (optional, none)");
  console.log("                       ├───── cgi_interpreter: The interpreter to use for the CGI script (optional, none)");
  console.log("                       ├───── cgi_timeout:     The timeout for the CGI script in seconds (optional, default: 1000)");
  console.log("                       └───── method: An array of allowed HTTP methods for this route (optional, default: [\"GET\", \"POST\"])");
  console.log("");
  console.info("Josn format shold be like this: (values as strings where applicable)");
  console.log("{");
  console.log("  \"servers\": [");
  console.log("    {");
  console.log("      ...");
  console.log("      ...");
  console.log("      ...");
  console.log("    }");
  console.log("    ...");
  console.log("  ]");
  console.log("}");
  return 2;
}

void json(std::string& configFileContent);
static int configFile(std::string const& file) {
  try {
    if (file.empty())
      throw std::runtime_error("no filename provided");

    std::ifstream configFileStream(std::string(".server/.config/" + file + ".json").c_str());
    if (!configFileStream)
      throw std::runtime_error("no such file " + file + ".json in .server/.config/ folder with read permissions");

    std::stringstream buffer;
    buffer << configFileStream.rdbuf();
    configFileStream.close();
    std::string configFileContent = buffer.str();

    if (configFileContent.empty())
      throw std::runtime_error("empty file");

    json(configFileContent);
    return 0;
  } catch (std::exception& e) {
    console.issue("Failed to load configuration file: " + std::string(e.what()));
    return 1;
  }
}

void generate(void);
static int autoConfig(void) {
  try {
    generate();
    return 0;
  } catch (std::exception& e) {
    console.issue("Failed to generate configuration file: " + std::string(e.what()));
    return 1;
  }
}

void loadPermissions(void);
int parse(int ac, char* av[]) {
  if (ac == 2) {
    loadPermissions();
    std::string arg = av[1];
    if (arg == "--help")
      return help();
    else if (arg == "--config-rules")
      return configRules();
    else if (arg.find("--config-file=") == 0)
      return configFile(arg.substr(14));
    else if (arg == "--auto-config")
      return autoConfig();
  }
  console.warning("Run './webserver --help' for more informations");
  return 2;
}