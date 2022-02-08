#ifndef IRC_IRCSERVER_HPP
#define IRC_IRCSERVER_HPP

#include "IRCserverInterface.hpp"
#include "User.hpp"
class Message;
class Channel;
#define _DELIM "\r\n"
#define _HOSTNAME_LEN 64
#define _NICKNAME_LEN 64


class IRCserver: public IRCserverInterface {
    typedef std::unordered_multimap<std::string, User> users;      // TODO: check if it works
    using userOperators =  std::map<std::string, User*>;
    using channels = std::map<std::string, Channel>;
public:
    void start() override;
private:
    int32_t         fdLimit;
    int32_t         listener;
    fd_set          userFdSet;
    std::string     buffer;
    std::string     dataDelimeter;
    users           usersAck;
    userOperators   userOpts;
    channels        userChannels;

    void    setHostname() override;
    void    setServerAdress() override;
    void    setMaxFd(int32_t maxFd);
    void    setListener(int32_t listener);
    void    setClientFds(const fd_set &clientFds);
    void    setBuffer(const std::string &buffer);
    void    setOperators(const userOperators &operators);
    void initListener();
    IRCserver();
private:
    void    serverShutdown() override;
    void    acceptConnection();
    bool    recieveData(int socket, std::string &buffer);
    bool    sendData(int socket, const std::string &buffer);
    void    addUser(int socket);
    void    addUser(const User &user);
    void    deleteUser(int socket);
    void    deleteUser(const std::string &nick);
    bool    isCorrectNickname(const std::string &nick);
    void    sendDataToJoinedChannels(const std::string &nick,
                                   const std::string &buffer);
    void    sendDataToChannel(const std::string &channel, const std::string &buffer, const std::string &nick = "");
    std::unordered_multimap<std::string, User>::iterator   getUserIter(int socket);
    void    _execute(int socket, const std::string &buffer);


    // TODO: make naming usual
    void    privateMessageCommand(const Message &msg, const User &usr);
    void    _CAP   (const Message &msg, const User &user);
    void    _PASS  (const Message &msg, User &user);
    void    _NICK  (const Message &msg, User **user);
    void    _USER  (const Message &msg, User &user);
    void    _PING  (const Message &msg, const User &user);
    void    _OPER  (const Message &msg, const User &user);
    void    _NOTICE(const Message &msg);
    void    _JOIN  (const Message &msg, User &usr);
    void    _PART  (const Message &msg, const User &usr);
    void    _OPER  (const Message &msg);
    void    _LIST  (const Message &msg, const User &user);
    void    _NAMES (const Message &msg, const User &user);
    void    quitCommad  (const Message &msg, User **user);
    void    _KILL  (const Message &msg, User **user);
    void    _KICK  (const Message &msg, const User &user);
    void    _TOPIC (const Message &msg, const User &user);
    void    _INVITE(const Message &msg, const User &user);

public:
    IRCserver(uint32_t port, std::string password);
    ~IRCserver() override;

    int32_t getMaxFd() const;
    int32_t getListener() const;
    const std::string&    getHostname() const override;
    sockaddr_in getServerAdress() const override;
    const fd_set &getClientFds() const;
    const std::string &getBuffer() const;
    const userOperators &getOperators() const;
    const channels &getChannels() const;
};


#endif //IRC_IRCSERVER_HPP
