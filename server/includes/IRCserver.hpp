#ifndef IRC_IRCSERVER_HPP
#define IRC_IRCSERVER_HPP

#include "IRCserverInterface.hpp"
#include "User.hpp"
#include "Channel.hpp"

class Message;
#define DELIM "\r\n"
#define HOSTNAME_LEN 64
#define NICKNAME_LEN 64


class IRCserver: public IRCserverInterface {
    using users = std::unordered_multimap<std::string, User>;
    using userOperators =  std::map<std::string, User*>;
    using channels = std::map<std::string, Channel>;
public:
    void start() override;
private:
    int32_t         fdLimit{};
    int32_t         listener{};
    fd_set          userFdSet{};
    std::string     buffer;
    std::string     dataDelimeter;
    users           usersAck;
    userOperators   userOpts;
    channels        userChannels;

    void    setHostname() override;
    void    setServerAddress() override;
    void    setMaxFd(int32_t maxFd);
    void    setListener(int32_t listener);
    void    setClientFds(const fd_set &clientFds);
    void    setBuffer(const std::string &buffer);
    void    setOperators(const userOperators &operators);
    void initListener();
    IRCserver() = default;;

private:
    void    serverShutdown() override;
    void    acceptConnection() override;
    bool    receiveData(int socket, std::string &buffer) override;
    bool    sendData(int socket, const std::string &buffer) override;
    void    addUser(int socket);
    void    addUser(const User &user);
    void    deleteUser(int socket);
    void    deleteUser(const std::string &nick);
    static  bool    isCorrectNickname(const std::string &nick);
    void    sendDataToJoinedChannels(const std::string &nick,
                                   const std::string &buf);
    void    sendDataToChannel(const std::string &channel, const std::string &buf, const std::string &nick = "");
    std::unordered_multimap<std::string, User>::iterator   getUserIter(int socket);
    void    executeCommand(int socket, const std::string &buffer);


    // TODO: make naming usual
    void    privateMessageCommand(const Message &msg, const User &usr);
    void    capCommand   (const Message &msg, const User &user);
    void    passCommand  (const Message &msg, User &user);
    void    nickCommand  (const Message &msg, User **user);
    void    userCommand  (const Message &msg, User &user);
    void    pingCommand  (const Message &msg, const User &user);
    void    operationCommand  (const Message &msg, const User &user);
    void    noticeCommand(const Message &msg);
    void    joinCommand  (const Message &msg, User &usr);
    void    partCommand  (const Message &msg, const User &usr);
    void    listCommand  (const Message &msg, const User &user);
    void    _NAMES (const Message &msg, const User &user);
    void    quitCommand  (const Message &msg, User **user);
    void    killCommand  (const Message &msg, User **user);
    void    _KICK  (const Message &msg, const User &user);
    void    topicCommand (const Message &msg, const User &user);
    void    inviteCommand(const Message &msg, const User &user);

public:
    IRCserver(int port, const std::string &passwd);
    ~IRCserver() override = default;

    int32_t getMaxFd() const;
    int32_t getListener() const;
    const std::string&    getHostname() const override;
    sockaddr_in getServerAddress() const override;
    const fd_set &getClientFds() const;
    const std::string &getBuffer() const;
    const userOperators &getOperators() const;
    const channels &getChannels() const;
};


#endif //IRC_IRCSERVER_HPP
