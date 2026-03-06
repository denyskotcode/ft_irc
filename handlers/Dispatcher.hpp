#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include "Handlers.hpp"

class Dispatcher
{
public:
    Dispatcher();
    Dispatcher(const Dispatcher& src);
    Dispatcher& operator=(const Dispatcher& rhs);
    ~Dispatcher();

    void dispatch_command(ServerState* state,
                          Handlers* handlers,
                          int clientFd,
                          const Message* cmd);
};

#endif
