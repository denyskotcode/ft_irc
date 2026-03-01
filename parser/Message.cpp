#include "Message.hpp"

Message::Message()
{

}

Message::~Message() 
{

}


Message::Message(const Message& src)
    : _prefix(src._prefix), _command(src._command), _params(src._params) {}

const std::string& Message::getPrefix() const
{
    return _prefix; 
}

const std::string& Message::getCommand() const
{ 
    return _command;
}

const std::vector<std::string>& Message::getParams() const
{
    return _params;
}

void Message::setPrefix(const std::string& prefix)
{
    _prefix = prefix;
}

void Message::setCommand(const std::string& command)
{
    _command = command;
}

void Message::addParam(const std::string& param)
{
    _params.push_back(param);
}

void Message::clear()
{
    _prefix.clear();
    _command.clear();
    _params.clear();
}

Message& Message::operator=(const Message& rhs)
{
    if (this != &rhs)
    {
        _prefix  = rhs._prefix;
        _command = rhs._command;
        _params  = rhs._params;
    }
    return *this;
}

