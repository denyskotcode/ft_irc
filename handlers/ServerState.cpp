#include "ServerState.hpp"

Client::Client()
    : fd(-1),
      passAccepted(false),
      hasNick(false),
      hasUser(false),
      registered(false),
      nickname(""),
      username(""),
      realname(""),
      hostname("localhost")
{
}

Channel::Channel()
    : name(""),
      topic(""),
      inviteOnly(false),
      topicOpOnly(true),
      hasKey(false),
      key(""),
      hasUserLimit(false),
      userLimit(0)
{
}

ServerState::ServerState(const std::string& serverNameValue, const std::string& passwordValue)
    : serverName(serverNameValue), password(passwordValue)
{
}
