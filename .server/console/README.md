## console global object:

**`init(int port = 3000, std::string network = "127.0.0.1", std::string name = "www", std::string version = "0.1.0")`**  
Initializes and displays the server startup banner with port information, local and network URLs.

**`success(char const* s)`**  
Displays a success message.

**`issue(char const* i)`**  
Displays an error/issue message.

**`info(char const* i)`**  
Displays an informational message.

**`warning(char const* w)`**  
Displays a warning message.

**`log(char const* l)`**  
Displays a plain log message.

**`GET(char const* path, int status, double in)`**  
Logs an HTTP GET request with the path, status code, and response time.

**`POST(char const* path, int status, double in)`**  
Logs an HTTP POST request with the path, status code, and response time.

**`DELETE(char const* path, int status, double in)`**  
Logs an HTTP DELETE request with the path, status code, and response time.