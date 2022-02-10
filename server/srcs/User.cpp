#include "../includes/User.hpp"

User::User():
        usePassword(false),
        useNick(false),
        activeUser(false),
        loggedNow(false),
        socket(-1)  {};

User::User(const User &rhs):
        nickname(rhs.nickname),
        actualName(rhs.actualName),
        serverName(rhs.serverName),
        usePassword(rhs.usePassword),
        useNick(rhs.useNick),
        activeUser(rhs.activeUser),
        loggedNow(rhs.loggedNow),
        socket(rhs.socket)    {};

User::~User() = default;


const std::string &User::getNickname() const {
    return nickname;
}

void User::setNickname(const std::string &nick) {
    nickname = nick;
}

const std::string &User::getUsername() const {
    return nickname;
}

void User::setUsername(const std::string &username) {
    actualName = username;
}

const std::string &User::getRealName() const {
    return actualName;
}

void User::setRealName(const std::string &realName) {
    actualName = realName;
}

const std::string &User::getServerName() const {
    return serverName;
}

const std::string &User::getBuffer() const {
    return buffer;
}

void User::setBuffer(const std::string &buf) {
    buffer = buf;
}

const std::string &User::getSendBuffer() const {
    return userSendBuffer;
}

void User::setUserSendBuffer(const std::string &send) {
    userSendBuffer = send;
}

bool User::hasPassword() const {
    return usePassword;
}

void User::setPassword(bool password) {
    usePassword = password;
}

bool User::hasNick() const {
    return useNick;
}

void User::setNick(bool nick) {
    useNick = nick;
}

bool User::isActiveUser() const {
    return activeUser;
}

void User::setUser(bool user) {
    activeUser = user;
}

bool User::isLogged() const {
    return loggedNow;
}


int User::getSocket() const {
    return socket;
}

void User::setSocket(int sock) {
    socket = sock;
}

void User::bufferClear()    {
    buffer.clear();
}

void User::bufferAppend(const std::string &data)    {
    buffer += data;
}

void User::enablePassword() {
    usePassword = true;
}

void User::enableLogged() {
    activeUser = true;
}

void User::enableUser() {
    activeUser = true;
}

void User::enableNickname() {
    useNick = true;
}


