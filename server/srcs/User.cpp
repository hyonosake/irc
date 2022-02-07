#include "User.hpp"

User::User():
        _socket(-1),
        _user(false),
        _password(false),
        _nick(false),
        _logged(false)  {};

User::User(const User &rhs):
        _username(rhs._username),
        _realName(rhs._realName),
        _nickname(rhs._nickname),
        _serverName(rhs._serverName),
        _password(rhs._password),
        _nick(rhs._nick),
        _user(rhs._user),
        _logged(rhs._logged),
        _socket(rhs._socket)    {};

User::~User() = default;


const std::string &User::getNickname() const {
    return _nickname;
}

void User::setNickname(const std::string &nickname) {
    _nickname = nickname;
}

const std::string &User::getUsername() const {
    return _username;
}

void User::setUsername(const std::string &username) {
    _username = username;
}

const std::string &User::getRealName() const {
    return _realName;
}

void User::setRealName(const std::string &realName) {
    _realName = realName;
}

const std::string &User::getServerName() const {
    return _serverName;
}

void User::setServerName(const std::string &serverName) {
    _serverName = serverName;
}

const std::string &User::getBuffer() const {
    return _buffer;
}

void User::setBuffer(const std::string &buffer) {
    _buffer = buffer;
}

const std::string &User::getSendBuffer() const {
    return _sendBuffer;
}

void User::setSendBuffer(const std::string &sendBuffer) {
    _sendBuffer = sendBuffer;
}

bool User::isPassword() const {
    return _password;
}

void User::setPassword(bool password) {
    _password = password;
}

bool User::isNick() const {
    return _nick;
}

void User::setNick(bool nick) {
    _nick = nick;
}

bool User::isUser() const {
    return _user;
}

void User::setUser(bool user) {
    _user = user;
}

bool User::isLogged() const {
    return _logged;
}

void User::setLogged(bool logged) {
    _logged = logged;
}

int User::getSocket() const {
    return _socket;
}

void User::setSocket(int socket) {
    _socket = socket;
}

void User::clearBuffer()    {
    _buffer.clear();
}

void User::appendBuffer(const std::string &data)    {
    _buffer += data;
}

User Users::getUser(int socket)  {

}

