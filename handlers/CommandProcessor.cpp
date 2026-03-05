#include "CommandProcessor.hpp"

CommandProcessor::CommandProcessor()
{
}

CommandProcessor::CommandProcessor(const CommandProcessor& src)
{
    (void)src;
}

CommandProcessor& CommandProcessor::operator=(const CommandProcessor& rhs)
{
    (void)rhs;
    return *this;
}

CommandProcessor::~CommandProcessor()
{
}

void CommandProcessor::processLine(ServerState* state,
                                   Dispatcher* dispatcher,
                                   Handlers* handlers,
                                   Parser* parser,
                                   int clientFd,
                                   const std::string& rawLine)
{
    if (!state || !dispatcher || !handlers || !parser)
        return;

    Message cmd = parser->parse(rawLine);
    if (cmd.getCommand().empty())
        return;

    dispatcher->dispatch_command(state, handlers, clientFd, &cmd);
}
