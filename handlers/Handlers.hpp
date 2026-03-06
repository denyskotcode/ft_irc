#ifndef HANDLERS_HPP
#define HANDLERS_HPP

#include "ServerState.hpp"
#include "State.hpp"
#include "../parser/Message.hpp"
#include <string>
#include <vector>

class Handlers
{
public:
    Handlers();
    Handlers(const Handlers& src);
    Handlers& operator=(const Handlers& rhs);
    ~Handlers();

    void cmd_pass(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_nick(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_user(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_join(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_privmsg(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_kick(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_invite(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_topic(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_mode(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_quit(ServerState* state, State* stateOps, Client* client, const Message* cmd);
    void cmd_cap(ServerState* state, Client* client, const Message* cmd);
    void cmd_ping(ServerState* state, Client* client, const Message* cmd);
    void cmd_unknown(ServerState* state, Client* client, const Message* cmd);

private:
    void maybeRegister(ServerState* state, Client* client);
    bool ensureRegistered(ServerState* state, Client* client);
    
    std::string intToString(int value);
    std::string nickOrStar(const Client* client);
    std::string userPrefix(const Client* client);
    std::string serverNumeric(const ServerState* state,
                             const Client* client,
                             const std::string& code,
                             const std::string& middle,
                             const std::string& trailing);
    bool isValidNickname(const std::string& nickname);
    bool isValidChannelName(const std::string& channelName);
    std::vector<std::string> splitByComma(const std::string& value);
    std::string buildNamesList(State* stateOps, const Channel* channel);
    std::string channelModeString(const Channel* channel);
};

#endif