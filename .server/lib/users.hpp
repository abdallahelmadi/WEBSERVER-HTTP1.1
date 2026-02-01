#pragma once

#include <string>
#include <map>
#include <cstdlib>

class UserManager {
private:
    std::map<std::string, std::string> users;

public:
    UserManager(){
        users["Guest"] = "admin1234";
    }

    bool checkLogin(const std::string& username, const std::string& password) const{
        std::map<std::string, std::string>::const_iterator it;

        it = users.find(username);
        if (it == users.end())
            return false;

        return it->second == password;
    }
    void addUser(const std::string& username, const std::string& password){
        users[username] = password;
    }
    static std::string generate_session_id(size_t length)
    {
        static const char hex[] = "0123456789abcdef";
        std::string id;
        for (size_t i = 0; i < length; ++i)
        {
            id += hex[std::rand() % 16];
        }
        return id;
    }

};
