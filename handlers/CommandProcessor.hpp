#ifndef COMMANDPROCESSOR_HPP
#define COMMANDPROCESSOR_HPP

#include "Dispatcher.hpp"
#include "../parser/Parser.hpp"
#include <string>

class CommandProcessor
{
public:
    CommandProcessor();
    CommandProcessor(const CommandProcessor& src);
    CommandProcessor& operator=(const CommandProcessor& rhs);
    ~CommandProcessor();

    void processLine(ServerState* state,
                     Dispatcher* dispatcher,
                     Handlers* handlers,
                     Parser* parser,
                     int clientFd,
                     const std::string& rawLine);
};

#endif
