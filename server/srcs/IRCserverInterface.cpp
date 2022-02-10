#include "../includes/IRCserverInterface.hpp"


IRCserverInterface::IRCserverInterface(int port, std::string password):
    serverPassword(password),
    serverPort(port) {}
