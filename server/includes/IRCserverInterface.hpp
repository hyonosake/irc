
#ifndef IRC_IRCSERVERINTERFACE_HPP
#define IRC_IRCSERVERINTERFACE_HPP

#include <algorithm>
#include <csignal>
#include <exception>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <vector>


#include <string>
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/select.h>
#include <sys/socket.h>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>

//#include "Channel.hpp"
//#include "Message.hpp"
//#include "User.hpp"
//#include "utils.hpp"

# define RESET   "\033[0m"
# define BLACK   "\033[30m"      /* Black */
# define RED     "\033[31m"      /* Red */
# define GREEN   "\033[32m"      /* Green */
# define YELLOW  "\033[33m"      /* Yellow */
# define BLUE    "\033[34m"      /* Blue */
# define MAGENTA "\033[35m"      /* Magenta */
# define CYAN    "\033[36m"      /* Cyan */
# define WHITE   "\033[37m"      /* White */


class IRCserverInterface {
protected:
    struct sockaddr_in               serverAddress{};
    std::string                      serverHostname;
    std::string                      serverPassword;
    int                              serverPort;

public:
    IRCserverInterface(int port, const std::string &passwd);
    virtual ~IRCserverInterface() = default;;
    virtual void    start() = 0;
protected:
    IRCserverInterface() = default;
    virtual void    serverShutdown() = 0;
    virtual void    acceptConnection() = 0;
    virtual bool    receiveData     (int socket, std::string &buffer) = 0;
    virtual bool    sendData      (int socket, const std::string &buffer) = 0;
    virtual void    setHostname() = 0;
    virtual const std::string &getHostname() const = 0;
    virtual void    setServerAddress() = 0;
    virtual sockaddr_in    getServerAddress() const = 0;
};


#endif //IRC_IRCSERVERINTERFACE_HPP
