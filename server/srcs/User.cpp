#include "User.hpp"

User::User():
        socket(-1),
        activeUser(false),
        usePassword(false),
        useNick(false),
        loggedNow(false)  {};

User::User(const User &rhs):
        userName(rhs.userName),
        actualName(rhs.actualName),
        userNickname(rhs.userNickname),
        serverName(rhs.serverName),
        usePassword(rhs.usePassword),
        useNick(rhs.useNick),
        activeUser(rhs.activeUser),
        loggedNow(rhs.loggedNow),
        socket(rhs.socket)    {};

User::~User() = default;


const std::string &User::getNickname() const {
    return userNickname;
}

void User::setNickname(const std::string &nickname) {
    userNickname = nickname;
}

const std::string &User::getUsername() const {
    return userName;
}

void User::setUsername(const std::string &username) {
    userName = username;
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

void User::setServerName(const std::string &serverName) {
    serverName = serverName;
}

const std::string &User::getBuffer() const {
    return buffer;
}

void User::setbuffer(const std::string &buffer) {
    buffer = buffer;
}

const std::string &User::getSendbuffer() const {
    return sendDatabuffer;
}

void User::setUserSendBuffer(const std::string &sendbuffer) {
    sendDatabuffer = sendbuffer;
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

void User::setLogged(bool logged) {
    loggedNow = logged;
}

int User::getSocket() const {
    return socket;
}

void User::setSocket(int socket) {
    socket = socket;
}

void User::bufferClear()    {
    buffer.clear();
}

void User::bufferAppend(const std::string &data)    {
    buffer += data;
}

bool User::hasPassworded() {
    return false;
}

User Users::getUser(int socket)  {

}

