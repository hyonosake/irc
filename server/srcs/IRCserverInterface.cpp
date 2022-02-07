#include "../includes/IRCserverInterface.hpp"


IRCserverInterface::IRCserverInterface(uint32_t port, std::string password):
    _port(port),
    _password(password) {}
