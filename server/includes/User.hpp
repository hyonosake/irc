
#ifndef IRC_USER_HPP
#define IRC_USER_HPP


#include <string>
#include <unordered_map>

class User {
public:
    User();
    User(const User &rhs);
    ~User();
private:
    std::string nickname;
    std::string actualName;
    std::string serverName;
    std::string buffer;
    std::string userSendBuffer;
    bool		usePassword;
    bool		useNick;
    bool		activeUser;
    bool		loggedNow;
    int			socket;
public:
    const std::string &getNickname() const;
    const std::string &getUsername() const;
    const std::string &getRealName() const;
    const std::string &getServerName() const;

    void setNickname(const std::string &nickname);
    void setRealName(const std::string &realName);
    void setUsername(const std::string &username);
    void setServerName(const std::string &serverName);
    void setPassword(bool password);
    void setNick(bool nick);
    void setUser(bool user);
    int getSocket() const;
    void setSocket(int socket);
    void enablePassword();
    bool hasPassword() const;
    bool hasNick() const;
    bool isActiveUser() const;
    bool isLogged() const;

    void				bufferClear(); // TODO: rename to bufferClear
    void				bufferAppend(const std::string &data);  // TODO: rename to bufferAppend
    const std::string	&getBuffer() const;   // TODO: rename to getBufferf
    void                setBuffer(const std::string &buffer);
    void				setUserSendBuffer(const std::string &data);  // TODO: rename to setbufferfSend
    const std::string	&getSendBuffer() const;   // TODO: rename to getBuffSend

    bool hasPassworded();

    void enableLogged();
    void enableNickname();
    void enableUser();
};
class Users {
public:
    std::unordered_multimap<std::string, User> users;
    User    getUser(int socket);
};

#endif //IRC_USER_HPP
