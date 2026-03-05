#ifndef SENDHELPERS_HPP
#define SENDHELPERS_HPP

#include "ServerState.hpp"
#include <string>

void send_to_client(ServerState* state, int clientFd, const std::string& line);
void broadcast_channel(ServerState* state,
                       const std::string& channelName,
                       const std::string& line,
                       int exceptFd);

#endif
