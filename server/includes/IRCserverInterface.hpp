
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

//void    sigintCatcher(int sig);

class IRCserverInterface {
protected:
    struct sockaddr_in               _serverAdress;
    std::string                      _hostname;
    std::string                      _password;
    uint32_t                          _port;

public:
    IRCserverInterface(uint32_t port, std::string password);
    virtual void    start() = 0;
protected:
    IRCserverInterface()    {};
    virtual ~IRCserverInterface() = 0;
    virtual void    _stop() = 0;
    virtual void    _accept() = 0;
    virtual bool    _recv      ( int sockfd,       std::string &buf ) = 0;
    virtual bool    _send      ( int sockfd, const std::string &buf ) = 0;
    virtual void    setHostname() = 0;
    virtual const std::string & getHostname() const = 0;
    virtual void    setServerAdress() = 0;
    virtual sockaddr_in&    getServerAdress() const = 0;
};


#endif //IRC_IRCSERVERINTERFACE_HPP
