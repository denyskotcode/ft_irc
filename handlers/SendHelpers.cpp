#include "SendHelpers.hpp"

void send_to_client(ServerState* state, int clientFd, const std::string& line)
{
    if (!state)
        return;
    std::map<int, Client>::iterator it = state->clients.find(clientFd);
    if (it == state->clients.end())
        return;
    it->second.outbox.push_back(line);
}

void broadcast_channel(ServerState* state,
                       const std::string& channelName,
                       const std::string& line,
                       int exceptFd)
{
    if (!state)
        return;
    std::map<std::string, Channel>::iterator it = state->channels.find(channelName);
    if (it == state->channels.end())
        return;

    std::set<int>::const_iterator fdIt = it->second.members.begin();
    for (; fdIt != it->second.members.end(); ++fdIt)
    {
        if (*fdIt == exceptFd)
            continue;
        send_to_client(state, *fdIt, line);
    }
}
