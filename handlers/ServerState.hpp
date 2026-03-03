#ifndef SERVERSTATE_HPP
#define SERVERSTATE_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

struct Client
{
    int fd;
    bool passAccepted;
    bool hasNick;
    bool hasUser;
    bool registered;
    std::string nickname;
    std::string username;
    std::string realname;
    std::string hostname;
    std::set<std::string> channels;
    std::vector<std::string> outbox;

    Client();
};

struct Channel
{
    std::string name;
    std::string topic;
    bool inviteOnly;
    bool topicOpOnly;
    bool hasKey;
    std::string key;
    bool hasUserLimit;
    std::size_t userLimit;
    std::set<int> members;
    std::set<int> operators;
    std::set<int> invited;

    Channel();
};

class ServerState
{
public:
    std::string serverName;
    std::string password;
    std::map<int, Client> clients;
    std::map<std::string, int> nickToFd;
    std::map<std::string, Channel> channels;

    ServerState(const std::string& serverNameValue, const std::string& passwordValue);
};

#endif
