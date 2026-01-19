#include <server.hpp>
#include <request.hpp>
#include <console.hpp>
#include <status.hpp>
#include <time.hpp>
#include <sys/socket.h>
#include <response.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include "post_helper.cpp"
#include <urlencoder.hpp>

std::string methodPost(int client, request& req, ctr& currentServer, long long startRequestTime) {
  // find matching route at config file
  rt* route = NULL;
  for (std::size_t i = 0; i < currentServer.length(); i++) {
    if (currentServer.route(i).path() == req.getPath()) {
      route = &currentServer.route(i);
      break;
    }
  }
  if (route == NULL) {
    std::map<std::string, std::string> headers;
    std::string body = "";
    return response(client, startRequestTime, 404, headers, body, req, currentServer).sendResponse();
  }
  if (
    route->method(0) != "POST" &&
    (route->length() > 1 && route->method(1) != "POST") &&
    (route->length() > 2 && route->method(2) != "POST")
  ) {
    std::map<std::string, std::string> Theaders;
    Theaders["Allow"] = "GET, POST, DELETE";
    Theaders["Content-Type"] = "text/html";
    return response(client, startRequestTime, 405, Theaders, "", req, currentServer).sendResponse();
  }

  std::cout << "im in post part now" << std::endl;
  if (!req.getBody().empty() && req.getHeaders().find("Content-Length") != req.getHeaders().end() && req.getHeaders().at("Content-Length") != "0") {
    std::string contentType = req.getHeaders().at("Content-Type");
    if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
    {
      std::string contentstored = req.getBody();
      urlencoder urlencoder;
      urlencoder.parseBodyContent(contentstored, currentServer);
      std::map<std::string, std::string> headers;
      std::string body = "Data received:\n" + contentstored;
      return response(client, startRequestTime, 200, headers, body, req, currentServer).sendResponse();
    }
    else if (contentType.find("multipart/form-data") != std::string::npos)
    {
      std::string contentbody = req.getBody();
      // std::cout << "show request body:\n" << contentbody << std::endl;
      if (handle_multipart(contentbody, req, currentServer) == -1) {
        std::map<std::string, std::string> headers;
        std::string body = "";
        return response(client, startRequestTime, 400, headers, body, req, currentServer).sendResponse();
      }
      std::map<std::string, std::string> headers;
      std::string body = "Multipart data received and processed.";
      return response(client, startRequestTime, 200, headers, body, req, currentServer).sendResponse();
    }
    else if (contentType.find("text/plain") != std::string::npos)
    {
      std::string contentstored = req.getBody();
      std::ofstream outfile((currentServer.uploaddir() + "plain_text.txt").c_str());
      outfile << contentstored;
      outfile.close();
      std::map<std::string, std::string> headers;
      std::string body = "Data received:\n" + contentstored;
      return response(client, startRequestTime, 200, headers, body, req, currentServer).sendResponse();
    }
    else if (contentType.find("application/json") != std::string::npos)
    {
      std::string content = req.getBody();
      handle_json(content, currentServer);
      std::map<std::string, std::string> headers;
      std::string body = "JSON Data received:\n" + content;
      return response(client, startRequestTime, 200, headers, body, req, currentServer).sendResponse();
    }
    else
    {
      std::stringstream ss;
      ss << "uploaded_file" << time::clock();
      std::string filename = ss.str();
      std::size_t slash_pos = contentType.find('/');
      if (slash_pos != std::string::npos) {
        filename += "." + contentType.substr(slash_pos + 1);
      }
      std::string filepath = currentServer.uploaddir() + filename;
      std::ofstream outfile(filepath.c_str(), std::ios::binary);
      outfile.write(req.getBody().c_str(), req.getBody().size());
      outfile.close();
      std::map<std::string, std::string> headers;
      std::string body = "Binary data received and saved as " + filename;
      return response(client, startRequestTime, 200, headers, body, req, currentServer).sendResponse();
    }
  }
  else if (req.getBody().empty()) {
    std::map<std::string, std::string> headers;
    return response(client, startRequestTime, 200, headers, "[]", req, currentServer).sendResponse();
  }
  else {
    std::map<std::string, std::string> headers;
    std::string body = "";
    return response(client, startRequestTime, 400, headers, body, req, currentServer).sendResponse();
  }
  return "";
}