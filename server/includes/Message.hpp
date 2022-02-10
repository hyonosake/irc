#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <iostream>

#include <sstream>
#include <vector>

#include "User.hpp"

class Message {
private:
    Message();
    bool							isMessagePrefix;
    bool							isCommand;
    bool							isColonSeparated(const std::string &str);
    bool							isCommaSeparated(const std::string &str);
public:
    Message(std::string, const User&);
    Message(const Message &rhs);
    ~Message();
    const std::string				&getPrefix() const;
    const std::string				&getCommand() const;
    const std::vector<std::string>	&getParamets() const;
    void							setCommand(const std::string &command);
    void                            printTest();
private:
    std::vector<std::string>		splitMessage(const std::string &str, char delimeter);
    void							parseMessage(std::string str, const User&);
    void							parseMessageUtility(std::vector<std::string>);
    std::string 			 		messagePrefix;
    std::vector<std::string>		messageParameters;
    std::string 			 		command;
};

#endif
