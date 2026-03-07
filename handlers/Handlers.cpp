#include "Handlers.hpp"
#include "SendHelpers.hpp"
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <vector>

std::string Handlers::intToString(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string Handlers::nickOrStar(const Client* client)
{
    if (!client || client->nickname.empty())
        return "*";
    return client->nickname;
}

std::string Handlers::userPrefix(const Client* client)
{
    std::string nick = nickOrStar(client);
    std::string user = client->username.empty() ? "unknown" : client->username;
    std::string host = client->hostname.empty() ? "localhost" : client->hostname;
    return ":" + nick + "!" + user + "@" + host;
}

std::string Handlers::serverNumeric(const ServerState* state,
                                    const Client* client,
                                    const std::string& code,
                                    const std::string& middle,
                                    const std::string& trailing)
{
    std::string line = ":" + state->serverName + " " + code + " " + nickOrStar(client);
    if (!middle.empty())
        line += " " + middle;
    if (!trailing.empty())
        line += " :" + trailing;
    line += "\r\n";
    return line;
}

bool Handlers::isValidNickname(const std::string& nickname)
{
    if (nickname.empty())
        return false;
    if (!std::isalpha(static_cast<unsigned char>(nickname[0])) &&
        nickname[0] != '_' && nickname[0] != '[' && nickname[0] != ']')
        return false;

    for (std::size_t i = 1; i < nickname.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(nickname[i]);
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '[' && c != ']')
            return false;
    }
    return true;
}

bool Handlers::isValidChannelName(const std::string& channelName)
{
    if (channelName.size() < 2)
        return false;
    if (channelName[0] != '#')
        return false;
    for (std::size_t i = 1; i < channelName.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(channelName[i]);
        if (std::isspace(c) || c == ',' || c == ':')
            return false;
    }
    return true;
}

std::vector<std::string> Handlers::splitByComma(const std::string& value)
{
    std::vector<std::string> out;
    std::string current;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == ',')
        {
            out.push_back(current);
            current.clear();
        }
        else
            current += value[i];
    }
    out.push_back(current);
    return out;
}

std::string Handlers::buildNamesList(State* stateOps, const Channel* channel)
{
    std::string names;
    std::set<int>::const_iterator it = channel->members.begin();
    for (; it != channel->members.end(); ++it)
    {
        const Client* member = stateOps->find_client_by_fd(*it);
        if (!member)
            continue;
        if (!names.empty())
            names += " ";
        if (channel->operators.find(*it) != channel->operators.end())
            names += "@";
        names += member->nickname;
    }
    return names;
}

std::string Handlers::channelModeString(const Channel* channel)
{
    std::string modes = "+";
    if (channel->inviteOnly)
        modes += "i";
    if (channel->topicOpOnly)
        modes += "t";
    if (channel->hasKey)
        modes += "k";
    if (channel->hasUserLimit)
        modes += "l";
    return modes;
}

Handlers::Handlers()
{
}

Handlers::Handlers(const Handlers& src)
{
    (void)src;
}

Handlers& Handlers::operator=(const Handlers& rhs)
{
    (void)rhs;
    return *this;
}

Handlers::~Handlers()
{
}

void Handlers::maybeRegister(ServerState* state, Client* client)
{
    if (!client->registered && client->passAccepted && client->hasNick && client->hasUser)
    {
        client->registered = true;
        
        send_to_client(state, client->fd, this->serverNumeric(state, client, "001", "", "Welcome to the Internet Relay Network " + client->nickname));
        
        send_to_client(state, client->fd, this->serverNumeric(state, client, "002", "", "Your host is " + state->serverName + ", running version 1.0"));
        
        send_to_client(state, client->fd, this->serverNumeric(state, client, "003", "", "This server was created recently"));
        
        send_to_client(state, client->fd, this->serverNumeric(state, client, "004", state->serverName + " 1.0 itkol oit", ""));
    }
}

bool Handlers::ensureRegistered(ServerState* state, Client* client)
{
    if (client->registered)
        return true;
    send_to_client(state, client->fd, this->serverNumeric(state, client, "451", "", "You have not registered"));
    return false;
}

void Handlers::cmd_pass(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    (void)stateOps;
    const std::vector<std::string>& params = cmd->getParams();
    if (client->registered)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "462", "", "You may not reregister"));
        return;
    }
    if (params.empty())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "461", "PASS", "Not enough parameters"));
        return;
    }
    if (state->password.empty() || params[0] == state->password)
        client->passAccepted = true;
    else
        send_to_client(state, client->fd, this->serverNumeric(state, client, "464", "", "Password incorrect"));
}

void Handlers::cmd_nick(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    const std::vector<std::string>& params = cmd->getParams();
    if (params.empty())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "431", "", "No nickname given"));
        return;
    }
    const std::string& newNick = params[0];
    if (!this->isValidNickname(newNick))
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "432", newNick, "Erroneous nickname"));
        return;
    }
    if (!stateOps->set_client_nick(client->fd, newNick))
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "433", newNick, "Nickname is already in use"));
        return;
    }
    maybeRegister(state, client);
}

void Handlers::cmd_user(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    (void)stateOps;
    const std::vector<std::string>& params = cmd->getParams();
    if (client->registered || client->hasUser)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "462", "", "You may not reregister"));
        return;
    }
    if (params.size() < 4)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "461", "USER", "Not enough parameters"));
        return;
    }

    client->username = params[0];
    client->realname = params[3];
    client->hasUser = true;
    maybeRegister(state, client);
}

void Handlers::cmd_join(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    const std::vector<std::string>& params = cmd->getParams();
    if (!ensureRegistered(state, client))
        return;
    if (params.empty())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "461", "JOIN", "Not enough parameters"));
        return;
    }

    std::vector<std::string> channelNames = this->splitByComma(params[0]);
    std::vector<std::string> keys;
    if (params.size() > 1)
        keys = this->splitByComma(params[1]);

    for (std::size_t i = 0; i < channelNames.size(); ++i)
    {
        const std::string& channelName = channelNames[i];
        if (!this->isValidChannelName(channelName))
        {
            send_to_client(state, client->fd, this->serverNumeric(state, client, "403", channelName, "No such channel"));
            continue;
        }

        Channel* channel = stateOps->create_channel(channelName);
        if (!channel)
            continue;

        if (channel->members.find(client->fd) != channel->members.end())
            continue;

        if (channel->inviteOnly && channel->invited.find(client->fd) == channel->invited.end())
        {
            send_to_client(state, client->fd, this->serverNumeric(state, client, "473", channelName, "Cannot join channel (+i)"));
            continue;
        }

        std::string providedKey;
        if (i < keys.size())
            providedKey = keys[i];
        if (channel->hasKey && channel->key != providedKey)
        {
            send_to_client(state, client->fd, this->serverNumeric(state, client, "475", channelName, "Cannot join channel (+k)"));
            continue;
        }

        if (channel->hasUserLimit && channel->members.size() >= channel->userLimit)
        {
            send_to_client(state, client->fd, this->serverNumeric(state, client, "471", channelName, "Cannot join channel (+l)"));
            continue;
        }

        stateOps->add_client_to_channel(client->fd, channelName);

        std::string joinLine = this->userPrefix(client) + " JOIN " + channelName + "\r\n";
        broadcast_channel(state, channelName, joinLine, -1);

        if (channel->topic.empty())
            send_to_client(state, client->fd, this->serverNumeric(state, client, "331", channelName, "No topic is set"));
        else
            send_to_client(state, client->fd, this->serverNumeric(state, client, "332", channelName, channel->topic));

        std::string names = this->buildNamesList(stateOps, channel);
        send_to_client(state, client->fd, this->serverNumeric(state, client, "353", "= " + channelName, names));
        send_to_client(state, client->fd, this->serverNumeric(state, client, "366", channelName, "End of /NAMES list"));
    }
}

void Handlers::cmd_privmsg(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    const std::vector<std::string>& params = cmd->getParams();
    if (!ensureRegistered(state, client))
        return;
    if (params.size() < 2)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "461", "PRIVMSG", "Not enough parameters"));
        return;
    }

    const std::string& target = params[0];
    const std::string& text = params[1];
    std::string line = this->userPrefix(client) + " PRIVMSG " + target + " :" + text + "\r\n";

    if (!target.empty() && target[0] == '#')
    {
        Channel* channel = stateOps->find_channel(target);
        if (!channel)
        {
            send_to_client(state, client->fd, this->serverNumeric(state, client, "403", target, "No such channel"));
            return;
        }
        if (channel->members.find(client->fd) == channel->members.end())
        {
            send_to_client(state, client->fd, this->serverNumeric(state, client, "404", target, "Cannot send to channel"));
            return;
        }
        broadcast_channel(state, target, line, client->fd);
        return;
    }

    Client* targetClient = stateOps->find_client_by_nick(target);
    if (!targetClient)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "401", target, "No such nick"));
        return;
    }
    send_to_client(state, targetClient->fd, line);
}

void Handlers::cmd_kick(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    const std::vector<std::string>& params = cmd->getParams();
    if (!ensureRegistered(state, client))
        return;
    if (params.size() < 2)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "461", "KICK", "Not enough parameters"));
        return;
    }

    const std::string& channelName = params[0];
    const std::string& targetNick = params[1];
    std::string reason = client->nickname;
    if (params.size() > 2)
        reason = params[2];

    Channel* channel = stateOps->find_channel(channelName);
    if (!channel)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "403", channelName, "No such channel"));
        return;
    }
    if (channel->members.find(client->fd) == channel->members.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "442", channelName, "You're not on that channel"));
        return;
    }
    if (channel->operators.find(client->fd) == channel->operators.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "482", channelName, "You're not channel operator"));
        return;
    }

    Client* target = stateOps->find_client_by_nick(targetNick);
    if (!target || channel->members.find(target->fd) == channel->members.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "441", targetNick + " " + channelName, "They aren't on that channel"));
        return;
    }

    std::string kickLine = this->userPrefix(client) + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    broadcast_channel(state, channelName, kickLine, -1);
    send_to_client(state, target->fd, kickLine);

    stateOps->remove_client_from_channel(target->fd, channelName);
    stateOps->erase_channel_if_empty(channelName);
}

void Handlers::cmd_invite(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    const std::vector<std::string>& params = cmd->getParams();
    if (!ensureRegistered(state, client))
        return;
    if (params.size() < 2)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "461", "INVITE", "Not enough parameters"));
        return;
    }

    const std::string& targetNick = params[0];
    const std::string& channelName = params[1];

    Channel* channel = stateOps->find_channel(channelName);
    if (!channel)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "403", channelName, "No such channel"));
        return;
    }
    if (channel->members.find(client->fd) == channel->members.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "442", channelName, "You're not on that channel"));
        return;
    }
    if (channel->operators.find(client->fd) == channel->operators.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "482", channelName, "You're not channel operator"));
        return;
    }

    Client* target = stateOps->find_client_by_nick(targetNick);
    if (!target)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "401", targetNick, "No such nick"));
        return;
    }
    if (channel->members.find(target->fd) != channel->members.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "443", targetNick + " " + channelName, "is already on channel"));
        return;
    }

    channel->invited.insert(target->fd);
    send_to_client(state, client->fd, this->serverNumeric(state, client, "341", targetNick + " " + channelName, ""));
    send_to_client(state, target->fd, this->userPrefix(client) + " INVITE " + targetNick + " :" + channelName + "\r\n");
}

void Handlers::cmd_topic(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    const std::vector<std::string>& params = cmd->getParams();
    if (!ensureRegistered(state, client))
        return;
    if (params.empty())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "461", "TOPIC", "Not enough parameters"));
        return;
    }

    const std::string& channelName = params[0];
    Channel* channel = stateOps->find_channel(channelName);
    if (!channel)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "403", channelName, "No such channel"));
        return;
    }
    if (channel->members.find(client->fd) == channel->members.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "442", channelName, "You're not on that channel"));
        return;
    }

    if (params.size() == 1)
    {
        if (channel->topic.empty())
            send_to_client(state, client->fd, this->serverNumeric(state, client, "331", channelName, "No topic is set"));
        else
            send_to_client(state, client->fd, this->serverNumeric(state, client, "332", channelName, channel->topic));
        return;
    }

    if (channel->topicOpOnly && channel->operators.find(client->fd) == channel->operators.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "482", channelName, "You're not channel operator"));
        return;
    }

    channel->topic = params[1];
    std::string topicLine = this->userPrefix(client) + " TOPIC " + channelName + " :" + channel->topic + "\r\n";
    broadcast_channel(state, channelName, topicLine, -1);
}

void Handlers::cmd_mode(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    const std::vector<std::string>& params = cmd->getParams();
    if (!ensureRegistered(state, client))
        return;
    if (params.empty())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "461", "MODE", "Not enough parameters"));
        return;
    }

    const std::string& channelName = params[0];
    Channel* channel = stateOps->find_channel(channelName);
    if (!channel)
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "403", channelName, "No such channel"));
        return;
    }

    if (params.size() == 1)
    {
        std::string line = ":" + state->serverName + " 324 " + this->nickOrStar(client) + " " + channelName + " " + this->channelModeString(channel);
        if (channel->hasKey)
            line += " " + channel->key;
        if (channel->hasUserLimit)
            line += " " + this->intToString(static_cast<int>(channel->userLimit));
        line += "\r\n";
        send_to_client(state, client->fd, line);
        return;
    }

    if (channel->operators.find(client->fd) == channel->operators.end())
    {
        send_to_client(state, client->fd, this->serverNumeric(state, client, "482", channelName, "You're not channel operator"));
        return;
    }

    const std::string& modeString = params[1];
    bool adding = true;
    std::size_t argIndex = 2;
    std::string appliedModes;
    std::vector<std::string> appliedArgs;

    for (std::size_t i = 0; i < modeString.size(); ++i)
    {
        char mode = modeString[i];
        if (mode == '+')
        {
            adding = true;
            appliedModes += '+';
            continue;
        }
        if (mode == '-')
        {
            adding = false;
            appliedModes += '-';
            continue;
        }

        if (mode == 'i')
            channel->inviteOnly = adding;
        else if (mode == 't')
            channel->topicOpOnly = adding;
        else if (mode == 'k')
        {
            if (adding)
            {
                if (argIndex >= params.size())
                    continue;
                channel->hasKey = true;
                channel->key = params[argIndex++];
                appliedArgs.push_back(channel->key);
            }
            else
            {
                channel->hasKey = false;
                channel->key.clear();
            }
        }
        else if (mode == 'l')
        {
            if (adding)
            {
                if (argIndex >= params.size())
                    continue;
                char* endPtr = 0;
                long limit = std::strtol(params[argIndex].c_str(), &endPtr, 10);
                if (*endPtr != '\0' || limit <= 0)
                {
                    ++argIndex;
                    continue;
                }
                channel->hasUserLimit = true;
                channel->userLimit = static_cast<std::size_t>(limit);
                appliedArgs.push_back(params[argIndex++]);
            }
            else
            {
                channel->hasUserLimit = false;
                channel->userLimit = 0;
            }
        }
        else if (mode == 'o')
        {
            if (argIndex >= params.size())
                continue;
            const std::string& targetNick = params[argIndex++];
            Client* target = stateOps->find_client_by_nick(targetNick);
            if (!target || channel->members.find(target->fd) == channel->members.end())
                continue;
            if (adding)
                channel->operators.insert(target->fd);
            else
                channel->operators.erase(target->fd);
            appliedArgs.push_back(targetNick);
        }
        else
            continue;

        appliedModes += mode;
    }

    if (appliedModes.empty() || appliedModes == "+" || appliedModes == "-")
        return;

    std::string line = this->userPrefix(client) + " MODE " + channelName + " " + appliedModes;
    for (std::size_t i = 0; i < appliedArgs.size(); ++i)
        line += " " + appliedArgs[i];
    line += "\r\n";
    broadcast_channel(state, channelName, line, -1);
}

void Handlers::cmd_quit(ServerState* state, State* stateOps, Client* client, const Message* cmd)
{
    const std::vector<std::string>& params = cmd->getParams();
    std::string reason = "Client Quit";
    if (!params.empty())
        reason = params[0];

    std::set<std::string> channels = client->channels;
    std::set<std::string>::const_iterator it = channels.begin();
    for (; it != channels.end(); ++it)
    {
        std::string quitLine = this->userPrefix(client) + " QUIT :" + reason + "\r\n";
        broadcast_channel(state, *it, quitLine, client->fd);
    }

    stateOps->remove_client(client->fd);
}

void Handlers::cmd_cap(ServerState* state, Client* client, const Message* cmd)
{
    // CAP is the IRC capability negotiation command sent by modern clients
    // (including WeeChat) before the normal PASS/NICK/USER registration flow.
    //
    // We don't support any capabilities, so we:
    //   - On "CAP LS"  → reply with an empty capability list, so the client
    //                     knows there is nothing to request and moves on.
    //   - On "CAP END" → acknowledge and do nothing (client ends negotiation).
    //   - Anything else → ignore silently (REQ, ACK etc. are not needed here).
    //
    // This is the minimum needed for WeeChat (and other clients) to proceed
    // with normal registration instead of hanging waiting for a CAP reply.

    const std::vector<std::string>& params = cmd->getParams();
    if (params.empty())
        return;

    const std::string& subcommand = params[0];

    if (subcommand == "LS")
    {
        // Tell the client: we support zero capabilities.
        // The empty trailing after the colon is intentional — it signals "no caps".
        send_to_client(state, client->fd,
            ":" + state->serverName + " CAP * LS :\r\n");
    }
    else if (subcommand == "END")
    {
        // Client is done negotiating — nothing to do on our side.
        // Registration will continue with PASS / NICK / USER.
        (void)0;
    }
    // REQ, ACK, NAK etc. — not needed since we advertise no capabilities.
}

void Handlers::cmd_ping(ServerState* state, Client* client, const Message* cmd)
{
    // PING is sent by the client (and by servers) to check the connection is alive.
    // We must reply with PONG carrying the same token the client sent.
    // Without this WeeChat will disconnect after its ping timeout.

    const std::vector<std::string>& params = cmd->getParams();
    std::string token = state->serverName; // default token if none provided
    if (!params.empty())
        token = params[0];

    send_to_client(state, client->fd,
        ":" + state->serverName + " PONG " + state->serverName + " :" + token + "\r\n");
}

void Handlers::cmd_unknown(ServerState* state, Client* client, const Message* cmd)
{
    send_to_client(state, client->fd, this->serverNumeric(state, client, "421", cmd->getCommand(), "Unknown command"));
}