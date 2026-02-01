#include <string>
#include <request.hpp>
#include <client.hpp>
#include <cstdlib>

void check_request_state(const std::string& requestdata, Client& clientObj) {
    //check for header complete
    if (!clientObj.header_complete) {
        std::size_t header_end = clientObj._request_data.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            clientObj.header_complete = true;
            std::size_t content_length_pos = requestdata.find("Content-Length:");
            if (content_length_pos != std::string::npos) {
                std::string length_str = requestdata.substr(content_length_pos + 15, requestdata.find("\r\n", content_length_pos) - (content_length_pos + 15));
                clientObj.content_length = static_cast<size_t>(std::atoi(length_str.c_str()));
            } else {
                clientObj.content_length = 0;
            }
        } else {
            return; // Header not complete yet
        }
    }
    //check for body complete
    if (clientObj.header_complete && !clientObj.body_complete) {
        std::size_t header_end = clientObj._request_data.find("\r\n\r\n");
        std::size_t body_start = header_end + 4;
        if (clientObj._request_data.length() - body_start >= clientObj.content_length) {
            clientObj.body_complete = true;
        }
        else {
            return; // Body not complete yet
        }
    }
}
int check_for_connection_close(const std::string& requestdata) {
    
    request req(requestdata);
    std::string connection;
    try {
        connection = req.getHeaders().at("Connection");
    } catch (const std::out_of_range& e) {
        connection = "keep-alive";
    }
    if (connection == "close")
    {
        return 1;
    }
    else
    {
        return 0;
    }
    return 0;
}
