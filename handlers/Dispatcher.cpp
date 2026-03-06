#include "Dispatcher.hpp"

Dispatcher::Dispatcher()
{
}

Dispatcher::Dispatcher(const Dispatcher& src)
{
    (void)src;
}

Dispatcher& Dispatcher::operator=(const Dispatcher& rhs)
{
    (void)rhs;
    return *this;
}

Dispatcher::~Dispatcher()
{
}

void Dispatcher::dispatch_command(ServerState* state,
                                  Handlers* handlers,
                                  int clientFd,
                                  const Message* cmd)
{
    if (!state || !handlers || !cmd)
        return;

    State stateOps(state);
    Client* client = stateOps.ensure_client(clientFd);
    if (!client)
        return;

    const std::string& command = cmd->getCommand();

    if (command == "PASS")
        handlers->cmd_pass(state, &stateOps, client, cmd);
    else if (command == "NICK")
        handlers->cmd_nick(state, &stateOps, client, cmd);
    else if (command == "USER")
        handlers->cmd_user(state, &stateOps, client, cmd);
    else if (command == "JOIN")
        handlers->cmd_join(state, &stateOps, client, cmd);
    else if (command == "PRIVMSG")
        handlers->cmd_privmsg(state, &stateOps, client, cmd);
    else if (command == "KICK")
        handlers->cmd_kick(state, &stateOps, client, cmd);
    else if (command == "INVITE")
        handlers->cmd_invite(state, &stateOps, client, cmd);
    else if (command == "TOPIC")
        handlers->cmd_topic(state, &stateOps, client, cmd);
    else if (command == "MODE")
        handlers->cmd_mode(state, &stateOps, client, cmd);
    else if (command == "QUIT")
        handlers->cmd_quit(state, &stateOps, client, cmd);
    else if (command == "CAP")
        handlers->cmd_cap(state, client, cmd);
    else if (command == "PING")
        handlers->cmd_ping(state, client, cmd);
    else
        handlers->cmd_unknown(state, client, cmd);
}