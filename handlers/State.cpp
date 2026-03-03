#include "State.hpp"

State::State(ServerState* state)
    : _state(state)
{
}

State::State(const State& src)
    : _state(src._state)
{
}

State& State::operator=(const State& rhs)
{
    if (this != &rhs)
        _state = rhs._state;
    return *this;
}

State::~State()
{
}

Client* State::ensure_client(int clientFd)
{
    if (!_state)
        return 0;
    std::map<int, Client>::iterator it = _state->clients.find(clientFd);
    if (it != _state->clients.end())
        return &it->second;

    Client client;
    client.fd = clientFd;
    if (_state->password.empty())
        client.passAccepted = true;
    _state->clients[clientFd] = client;
    return &_state->clients[clientFd];
}

Client* State::find_client_by_fd(int clientFd)
{
    if (!_state)
        return 0;
    std::map<int, Client>::iterator it = _state->clients.find(clientFd);
    if (it == _state->clients.end())
        return 0;
    return &it->second;
}

const Client* State::find_client_by_fd(int clientFd) const
{
    if (!_state)
        return 0;
    std::map<int, Client>::const_iterator it = _state->clients.find(clientFd);
    if (it == _state->clients.end())
        return 0;
    return &it->second;
}

Client* State::find_client_by_nick(const std::string& nickname)
{
    if (!_state)
        return 0;
    std::map<std::string, int>::iterator it = _state->nickToFd.find(nickname);
    if (it == _state->nickToFd.end())
        return 0;
    return find_client_by_fd(it->second);
}

const Client* State::find_client_by_nick(const std::string& nickname) const
{
    if (!_state)
        return 0;
    std::map<std::string, int>::const_iterator it = _state->nickToFd.find(nickname);
    if (it == _state->nickToFd.end())
        return 0;
    return find_client_by_fd(it->second);
}

Channel* State::find_channel(const std::string& channelName)
{
    if (!_state)
        return 0;
    std::map<std::string, Channel>::iterator it = _state->channels.find(channelName);
    if (it == _state->channels.end())
        return 0;
    return &it->second;
}

const Channel* State::find_channel(const std::string& channelName) const
{
    if (!_state)
        return 0;
    std::map<std::string, Channel>::const_iterator it = _state->channels.find(channelName);
    if (it == _state->channels.end())
        return 0;
    return &it->second;
}

Channel* State::create_channel(const std::string& channelName)
{
    if (!_state)
        return 0;
    std::map<std::string, Channel>::iterator it = _state->channels.find(channelName);
    if (it != _state->channels.end())
        return &it->second;

    Channel channel;
    channel.name = channelName;
    _state->channels[channelName] = channel;
    return &_state->channels[channelName];
}

bool State::set_client_nick(int clientFd, const std::string& nickname)
{
    if (!_state)
        return false;
    std::map<std::string, int>::iterator existing = _state->nickToFd.find(nickname);
    if (existing != _state->nickToFd.end() && existing->second != clientFd)
        return false;

    Client* client = find_client_by_fd(clientFd);
    if (!client)
        return false;

    if (!client->nickname.empty())
        _state->nickToFd.erase(client->nickname);
    client->nickname = nickname;
    client->hasNick = true;
    _state->nickToFd[nickname] = clientFd;
    return true;
}

void State::add_client_to_channel(int clientFd, const std::string& channelName)
{
    Client* client = find_client_by_fd(clientFd);
    Channel* channel = create_channel(channelName);
    if (!client || !channel)
        return;

    channel->members.insert(clientFd);
    client->channels.insert(channelName);
    channel->invited.erase(clientFd);
    if (channel->members.size() == 1)
        channel->operators.insert(clientFd);
}

void State::remove_client_from_channel(int clientFd, const std::string& channelName)
{
    Client* client = find_client_by_fd(clientFd);
    Channel* channel = find_channel(channelName);
    if (!client || !channel)
        return;

    channel->members.erase(clientFd);
    channel->operators.erase(clientFd);
    channel->invited.erase(clientFd);
    client->channels.erase(channelName);
}

void State::remove_client_from_all_channels(int clientFd)
{
    Client* client = find_client_by_fd(clientFd);
    if (!client)
        return;

    std::set<std::string> joined = client->channels;
    std::set<std::string>::const_iterator it = joined.begin();
    for (; it != joined.end(); ++it)
    {
        remove_client_from_channel(clientFd, *it);
        erase_channel_if_empty(*it);
    }
}

void State::erase_channel_if_empty(const std::string& channelName)
{
    if (!_state)
        return;
    Channel* channel = find_channel(channelName);
    if (!channel)
        return;
    if (channel->members.empty())
        _state->channels.erase(channelName);
}

void State::remove_client(int clientFd)
{
    if (!_state)
        return;
    Client* client = find_client_by_fd(clientFd);
    if (!client)
        return;

    if (!client->nickname.empty())
        _state->nickToFd.erase(client->nickname);

    remove_client_from_all_channels(clientFd);
    _state->clients.erase(clientFd);
}
