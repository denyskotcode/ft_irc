#include "Parser.hpp"
#include <cctype>


Parser::Parser() {}

Parser::Parser(const Parser& src)
{ 
    (void)src;
}

Parser& Parser::operator=(const Parser& rhs)
{
    (void)rhs;
    return *this;
}

Parser::~Parser() {}


std::string Parser::stripCRLF(const std::string& s) const
{
    std::string result = s;
    while (!result.empty() &&
           (result[result.size() - 1] == '\r' ||
            result[result.size() - 1] == '\n'))
        result.erase(result.size() - 1);
    return result;
}

std::string Parser::toUpper(const std::string& s) const
{
    std::string result = s;
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(result[i])));
    return result;
}

Message Parser::parse(const std::string& raw_line) const
{
    Message msg;
    std::string line = stripCRLF(raw_line);

    if (line.empty())
        return msg;

    std::size_t pos = 0;

    if (line[pos] == ':')
    {
        std::size_t space = line.find(' ', pos);
        if (space == std::string::npos)
            return msg;
        msg.setPrefix(line.substr(1, space - 1));
        pos = space + 1;
        while (pos < line.size() && line[pos] == ' ')
            ++pos;
    }

    if (pos >= line.size())
        return msg;
    {
        std::size_t space = line.find(' ', pos);
        if (space == std::string::npos)
        {
            msg.setCommand(toUpper(line.substr(pos)));
            return msg;
        }
        msg.setCommand(toUpper(line.substr(pos, space - pos)));
        pos = space + 1;
    }

    while (pos < line.size())
    {
        while (pos < line.size() && line[pos] == ' ')
            ++pos;

        if (pos >= line.size())
            break;

        if (line[pos] == ':')
        {
            msg.addParam(line.substr(pos + 1));
            break;
        }

        std::size_t space = line.find(' ', pos);
        if (space == std::string::npos)
        {
            msg.addParam(line.substr(pos));
            break;
        }
        msg.addParam(line.substr(pos, space - pos));
        pos = space + 1;
    }

    return msg;
}