#include "../includes/IRCserverInterface.hpp"


IRCserverInterface::IRCserverInterface(uint32_t port, std::string password):
    serverPort(port),
    serverPassword(password) {}
