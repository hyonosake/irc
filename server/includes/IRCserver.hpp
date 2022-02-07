#ifndef IRC_IRCSERVER_HPP
#define IRC_IRCSERVER_HPP

#include "IRCserverInterface.hpp"
#include "User.hpp"
class Message;
//class User;
class Channel;
#define _DELIM "\r\n"
#define _HOSTNAME_LEN 64


class IRCserver: public IRCserverInterface {
    typedef std::unordered_multimap<std::string, *User> users;      // TODO: check if it works
    using userOperators =  std::map<std::string, User*>;
    using channels = std::map<std::string, Channel>;
public:
    void start() override;
private:
    int32_t         _max_fd;
    int32_t         _listener;
    fd_set          _client_fds;
    std::string     _buffer;
    std::string     _delimeter;
    users           _users;
    userOperators   _operators;
    channels        _channels;

    void    setHostname() override;
    void    setServerAdress() override;
    void setMaxFd(int32_t maxFd);
    void setListener(int32_t listener);
    void setClientFds(const fd_set &clientFds);
    void setBuffer(const std::string &buffer);
    void setOperators(const userOperators &operators);
    void initListener();
    IRCserver();
private:
    void    _stop();
    void    _accept();
    bool    _recv(int sockfd, std::string &buf);
    bool    _send(int sockfd, const std::string &buf);
    void    _addUser   (int sockfd);
    void    _addUser   ( const User &user   );
    void    _removeUser( int sockfd         );
    void    _removeUser( const std::string &nick );
    bool    _isCorrectNick( const std::string &nick );
    void    _sendToJoinedChannels( const std::string &nick,
                                   const std::string &buf );

    void    _sendToChannel( const std::string &channel,
                            const std::string &buf,
                            const std::string &nick = "" );
    std::multimap<std::string, User>::iterator    _getUser( int sockfd );
    void    _execute( int sockfd, const std::string &buf );


    // TODO: make naming usual
    void    _PRIVMSG( const Message &msg, const User &usr);
    void    _CAP    ( const Message &msg, const User &user );
    void    _PASS   ( const Message &msg, User &user );
    void    _NICK   ( const Message &msg, User **user );
    void    _USER   ( const Message &msg, User &user );

    void    _PING   ( const Message &msg, const User &user );
    void    _OPER   ( const Message &msg, const User &user );
    void    _NOTICE (const Message &msg);
    void    _JOIN   (const Message &msg, User &usr);
    void    _PART   (const Message &msg, const User &usr);
    void    _OPER   (const Message &msg);
    void    _LIST   (const Message &msg, const User &user);
    void    _NAMES  (const Message &msg, const User &user);
    void    _QUIT   ( const Message &msg, User **user );
    void    _KILL   ( const Message &msg, User **user );
    void    _KICK   ( const Message &msg, const User &user );

    void    _TOPIC  (const Message &msg, const User &user);
    void    _INVITE (const Message &msg, const User &user);

public:
    IRCserver(uint32_t port, std::string password);
    ~IRCserver() override;

    int32_t getMaxFd() const;
    int32_t getListener() const;
    const std::string&    getHostname() const override;
    sockaddr_in & getServerAdress() const override;
    const fd_set &getClientFds() const;
    const std::string &getBuffer() const;
    const userOperators &getOperators() const;
    const channels &getChannels() const;
};


#endif //IRC_IRCSERVER_HPP
