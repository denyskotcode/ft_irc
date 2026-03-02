#ifndef PARSER_HPP
#define PARSER_HPP

#include "Message.hpp"
#include <string>

class Parser
{
public:
    Parser();
    Parser(const Parser& src);
    Parser& operator=(const Parser& rhs);
    ~Parser();

    Message parse(const std::string& raw_line) const;

private:
    std::string stripCRLF(const std::string& s) const;
    std::string toUpper(const std::string& s) const;
};

#endif
