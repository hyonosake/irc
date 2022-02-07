
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
    std::string _nickname;
    std::string _username;
    std::string _realName;
    std::string _serverName;
    std::string _buffer;
    std::string _sendBuffer;
    bool		_password;
    bool		_nick;
    bool		_user;
    bool		_logged;
    int			_socket;
public:
    const std::string &getNickname() const;
    const std::string &getUsername() const;
    const std::string &getRealName() const;
    const std::string &getServerName() const;

    void setNickname(const std::string &nickname);
    void setRealName(const std::string &realName);
    void setUsername(const std::string &username);
    void setServerName(const std::string &serverName);
    void setBuffer(const std::string &buffer);
    void setPassword(bool password);
    void setNick(bool nick);
    void setUser(bool user);
    void setLogged(bool logged);
    int getSocket() const;
    void setSocket(int socket);

    bool isPassword() const;
    bool isNick() const;
    bool isUser() const;
    bool isLogged() const;

    void				clearBuffer		(); // TODO: rename to bufClear
    void				appendBuffer	(const std::string &data);  // TODO: rename to bufAppend
    const std::string	&getBuffer		() const;   // TODO: rename to getBuff
    void				setSendBuffer	(const std::string &data);  // TODO: rename to setBuffSend
    const std::string	&getSendBuffer	() const;   // TODO: rename to getBuffSend

};
class Users {
public:
    std::unordered_multimap<std::string, User> users;
    User    getUser(int socket)
};

#endif //IRC_USER_HPP
