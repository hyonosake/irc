#include "../includes/Message.hpp"

Message::Message() {}

Message::Message(const Message &rhs) : messagePrefix(rhs.messagePrefix),
                                       messageParameters(rhs.messageParameters),
                                       command(rhs.command),
                                       isMessagePrefix(rhs.isMessagePrefix),
                                       isCommand(rhs.isCommand)
{
}

Message::~Message() {}

Message::Message(std::string str, const User &usr)
    : isMessagePrefix(false), isCommand(false)
{
    parseMessage(str, usr);
}

void Message::setCommand(const std::string &command)
{
    command = command;
}

const std::string &Message::getCommand() const { return command; }

const std::vector<std::string> &Message::getParamets() const { return messageParameters; }

const std::string &Message::getPrefix() const { return messagePrefix; }

void Message::parseMessage(std::string str, const User &usr)
{
    std::vector<std::string> vec_sep_space;
    std::vector<std::string> vec_sep_colon;

    messagePrefix = usr.getNickname();

    if (str[0] == ':')
        isMessagePrefix = true;

    if (isColonSeparated(str))
    {
        std::string buffer_str;

        vec_sep_colon = splitMessage(str, ':');
        for (size_t i = 1; i < vec_sep_colon.size(); ++i)
            buffer_str += vec_sep_colon[i];
        vec_sep_space = splitMessage(vec_sep_colon[0], ' ');
        parseMessageUtility(vec_sep_space);
        messageParameters.push_back(buffer_str);
    }
    else
    {
        vec_sep_space = splitMessage(str, ' ');
        parseMessageUtility(vec_sep_space);
    }
}

std::vector<std::string> Message::splitMessage(const std::string &str, char delimeter)
{
    std::vector<std::string> result;
    std::istringstream sstream(str);
    std::string tmp;

    while (std::getline(sstream, tmp, delimeter))
    {
        if (!tmp.empty())
            result.push_back(tmp);
    }
    return result;
}

bool Message::isColonSeparated(const std::string &str)
{
    for (size_t i = 0; i != str.size(); ++i)
    {
        if (str[i] == ':')
            return true;
    }
    return false;
}

bool Message::isCommaSeparated(const std::string &str)
{
    for (size_t i = 0; i != str.size(); ++i)
    {
        if (str[i] == ',')
            return true;
    }
    return false;
}

void Message::parseMessageUtility(std::vector<std::string> vec_sep_space)
{
    std::vector<std::string> vec_sep_comma;

    if (!vec_sep_space.empty())
    {
        for (size_t i = 0; i != vec_sep_space.size(); ++i)
        {
            if (isMessagePrefix && i == 0)
            {
                messagePrefix = vec_sep_space[i];
            }
            else if (!isCommand)
            {
                command = vec_sep_space[i];
                isCommand = true;
            }
            else
            {
                if (isCommaSeparated(vec_sep_space[i]))
                {
                    vec_sep_comma = splitMessage(vec_sep_space[i], ',');
                    for (size_t j = 0; j < vec_sep_comma.size(); ++j)
                        messageParameters.push_back(vec_sep_comma[j]);
                }
                else
                    messageParameters.push_back(vec_sep_space[i]);
            }
        }
    }
}

void Message::_printTest()
{
    std::cout << std::endl;

    std::cout << "PREFIX: " << std::endl
              << messagePrefix << std::endl;

    std::cout << "CMD: " << std::endl
              << command << std::endl;

    std::cout << "PARAMETERS: " << std::endl;
    for (size_t i = 0; i < messageParameters.size(); ++i)
    {
        std::cout << messageParameters[i] << std::endl;
    }
}
