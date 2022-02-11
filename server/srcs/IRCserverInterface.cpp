#include "../includes/IRCserverInterface.hpp"

IRCserverInterface::IRCserverInterface(int port, const std::string &passwd):
    serverPassword(passwd),
    serverPort(port) {}
