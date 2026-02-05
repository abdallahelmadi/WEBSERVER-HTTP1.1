#include <server.hpp>
#include <request.hpp>
#include <console.hpp>
#include <status.hpp>
#include <time.hpp>
#include <sys/socket.h>
#include <response.hpp>
#include <string>
#include <path.hpp>
#include <fstream>
#include <sstream>
#include "post_helper.cpp"
#include <urlencoder.hpp>
#include <users.hpp>
#include <client.hpp>

std::string methodPost(int client, request& req, ctr& currentServer, long long startRequestTime, Client &clientObj, UserManager &userManager) {
  // find matching route at config file

  /*******************************************************************/
  /*******************************************************************/
  if(!req.getBody().empty() && req.getBody().find("username") != std::string::npos && req.getBody().find("password") != std::string::npos) {
    std::string username;
    std::string password;
    std::string action;
    std::size_t user_pos = req.getBody().find("username=");
    std::size_t pass_pos = req.getBody().find("password=");
    std::size_t pass_end = req.getBody().find("&", pass_pos);
    if (pass_end == std::string::npos)
        pass_end = req.getBody().length();
    std::size_t action_pos = req.getBody().find("action=");
    std::size_t user_end = req.getBody().find("&", user_pos);

    username = req.getBody().substr(user_pos + 9, user_end - (user_pos + 9));
    // std::size_t pass_end = req.getBody().length();
    password = req.getBody().substr(pass_pos + 9, pass_end - (pass_pos + 9));

    if(action_pos != std::string::npos) {
      std::size_t action_end = req.getBody().length();
      action = req.getBody().substr(action_pos + 7, action_end - (action_pos + 7));
    }

    // std::cout << "Login attempt with Username: " << username << " and Password: " << password << std::endl;

    //check if not register request not set 
    if(action_pos == std::string::npos){
      if (userManager.checkLogin(username, password)) {
        std::map<std::string, std::string> headers;
        headers["location"] = "/profile?success=1";
        headers["Cache-Control"] = "no-store";
        // set cookie header !
        headers["Set-Cookie"] = "sessionid=" + UserManager::generate_session_id(16);
        clientObj._has_logged_in = true;
        return response(client, startRequestTime, 302, headers, "", req, currentServer).sendResponse();
      } else {
        std::map<std::string, std::string> headers;
        headers["location"] = "/login?error=1";
        headers["Cache-Control"] = "no-store";
        return response(client, startRequestTime, 302, headers, "", req, currentServer).sendResponse();
      }
    }
    // handle register request
    userManager.addUser(username, password);
    std::map<std::string, std::string> headers;
    headers["location"] = "/login?registered=1";
    headers["Cache-Control"] = "no-store";
    // userManager.printUsers();
    return response(client, startRequestTime, 302, headers, "", req, currentServer).sendResponse();
  }
  /*******************************************************************/
  /*******************************************************************/

  rt* route = NULL;
  for (std::size_t i = 0; i < currentServer.length(); i++) {
    if (path::check(currentServer.route(i).path(), req.getPath())) {
      route = &currentServer.route(i);
      break;
    }
  }
  if (route == NULL) {
    std::map<std::string, std::string> headers;
    std::string body = "";
    return response(client, startRequestTime, 404, headers, body, req, currentServer).sendResponse();
  }

  for (std::size_t i = 0; i < route->length(); i++) {
    if (route->method(i) == "POST") {
      break;
    }
    if (i == route->length() - 1) {
      std::map<std::string, std::string> Theaders;
      Theaders["Allow"] = "GET, POST, DELETE";
      Theaders["Content-Type"] = "text/html";
      return response(client, startRequestTime, 405, Theaders, "", req, currentServer).sendResponse();
    }
  }

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
      std::ofstream outfile((currentServer.root() + generate_uiid(6) + "-" + "plain_text.txt").c_str());
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
      std::string filepath = currentServer.root() + generate_uiid(6) + "-" + filename;
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