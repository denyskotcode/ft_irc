#ifndef STATE_HPP
#define STATE_HPP

#include "ServerState.hpp"

class State
{
private:
    ServerState* _state;

public:
    State(ServerState* state);
    State(const State& src);
    State& operator=(const State& rhs);
    ~State();

    Client* ensure_client(int clientFd);
    Client* find_client_by_fd(int clientFd);
    const Client* find_client_by_fd(int clientFd) const;
    Client* find_client_by_nick(const std::string& nickname);
    const Client* find_client_by_nick(const std::string& nickname) const;

    Channel* find_channel(const std::string& channelName);
    const Channel* find_channel(const std::string& channelName) const;
    Channel* create_channel(const std::string& channelName);

    bool set_client_nick(int clientFd, const std::string& nickname);
    void add_client_to_channel(int clientFd, const std::string& channelName);
    void remove_client_from_channel(int clientFd, const std::string& channelName);
    void remove_client_from_all_channels(int clientFd);
    void erase_channel_if_empty(const std::string& channelName);
    void remove_client(int clientFd);
};

#endif
