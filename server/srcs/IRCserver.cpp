#include "../includes/IRCserver.hpp"

#include <utility>
#include "../includes/Message.hpp"


static bool exitFlag = false;   // TODO: rename

void    sigintCatcher(int sig)  {
    if (sig == SIGINT)
        exitFlag = true;
}

IRCserver::IRCserver(int port, std::string passwd):
    IRCserverInterface(port, std::move(passwd))  {
    dataDelimeter = _DELIM;
    setHostname();
    setServerAddress();
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sigintCatcher); // TODO: sigintCatcher
    initListener();
}

int32_t IRCserver::getMaxFd() const {
    return fdLimit;
}

void IRCserver::setMaxFd(int32_t maxFd) {
    fdLimit = maxFd;
}

int32_t IRCserver::getListener() const {
    return listener;
}

void IRCserver::setListener(int32_t setlist) {
    listener = setlist;
}

const fd_set &IRCserver::getClientFds() const {
    return userFdSet;
}

void IRCserver::setClientFds(const fd_set &clientFds) {
    userFdSet = clientFds;
}

const std::string &IRCserver::getBuffer() const {
    return buffer;
}

void IRCserver::setBuffer(const std::string &buff) {
    buffer = buff;
}

const std::map<std::string, User *> &IRCserver::getOperators() const {
    return userOpts;
}

void IRCserver::setOperators(const std::map<std::string, User *> &operators) {
    userOpts = operators;
}

const std::map<std::string, Channel> &IRCserver::getChannels() const {
    return userChannels;
}

const std::string &IRCserver::getHostname() const {
    return serverHostname;
}

void IRCserver::setHostname() {
    char hostname[_HOSTNAME_LEN];
    bzero(static_cast<void*>(hostname), _HOSTNAME_LEN);
    if(gethostname(hostname, _HOSTNAME_LEN) != -1)
        serverHostname = hostname;
}

void IRCserver::setServerAddress() {
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(serverPort);
}

void IRCserver::initListener() {

    FD_ZERO(&userFdSet);
    listener = socket(AF_INET, SOCK_STREAM, getprotobyname("TCP")->p_proto);
    if (listener < 0)
        throw std::invalid_argument(strerror(errno));
    fdLimit = listener;

    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    int b = bind(listener, (struct sockaddr *)(&serverAddress), sizeof(serverAddress));
    if (b < 0)
        throw std::invalid_argument(strerror(errno));
    int l = listen(listener, 10);
    if (l < 0)
        throw std::invalid_argument(strerror(errno));

    FD_SET(listener, &userFdSet);
    fdLimit = listener;
};

void IRCserver::acceptConnection()  {
    sockaddr_in temp;
    socklen_t socklen = sizeof(struct sockaddr_in);
    int new_fd = accept(listener,(struct sockaddr *)&temp, &socklen);
    if (new_fd == -1)
        throw std::invalid_argument(strerror(errno));
    fcntl(listener, F_SETFL, O_NONBLOCK);
    addUser(new_fd);
    if (fdLimit < new_fd)
        fdLimit = new_fd;
    FD_SET(new_fd, &userFdSet);
    std::cout << CYAN << "New connection on socket " << new_fd << RESET << '\n';   // TODO: change message
}

void IRCserver::addUser(int socket) {
    User created;
    created.setSocket(socket);
    usersAck.insert({"default", created});
}

void IRCserver::addUser(const User &user)   {
    std::pair<std::string, User> tmp(user.getNickname(), user);
    usersAck.insert({user.getNickname(), user});
}

void IRCserver::start() {
    fd_set select_fds;
    select_fds = userFdSet;
    std::cout << GREEN << "Server started..." << RESET << std::endl; // TODO: change message
    while (select(fdLimit + 1, &select_fds, nullptr, nullptr, nullptr) != -1)    {
        if (exitFlag)
            serverShutdown();
        for (auto i = 3; i < fdLimit + 1; i++)   {
            if (!FD_ISSET(i, &select_fds) || i == listener)
                continue;
            std::string localBuffer;
            auto usrIterator = getUserIter(i);
            if (usrIterator == usersAck.end())
                continue;
            if (usrIterator->second.getSendBuffer().length())
                sendData(i, usrIterator->second.getSendBuffer());
            try {
                receiveData(i, localBuffer);
            } catch (const std::exception &e)   {
                auto msg = Message("QUIT :Remote host closed the connection", usrIterator->second);
                quitCommand(msg, reinterpret_cast<User **>(&usrIterator->second));
            }
            executeCommand(i, buffer);
            std::cout << "Number of users : "<< usersAck.size() << '\n';
            int logged = 0;
            for (usrIterator = usersAck.begin(); usrIterator != usersAck.end(); ++usrIterator)
                if (usrIterator->second.isLogged())
                    logged++;
            std::cout << "Number of logged users : " << logged << '\n';
        }
        if (FD_ISSET(listener, &select_fds))
            acceptConnection();
        select_fds = userFdSet;
    }
    FD_ZERO(&select_fds);
    FD_ZERO(&userFdSet);
    throw std::invalid_argument(strerror(errno));
}


void    IRCserver::serverShutdown() {
    std::unordered_multimap<std::string, User>::iterator    it;
    for (it = usersAck.begin(); it != usersAck.end(); ++it) {
        Message msg = Message("Stopped: ", it->second);
        quitCommand(msg, reinterpret_cast<User **>(&it->second));
    }
    FD_ZERO(&userFdSet);
}

std::unordered_multimap<std::string, User>::iterator IRCserver::getUserIter(int socket) {
    for (auto it = usersAck.begin(); it != usersAck.end(); ++it)  {
        if (it->second.getSocket() == socket)
            return it;
    }
    return usersAck.end();
}

void IRCserver::deleteUser(const std::string &remname) {
    usersAck.erase(remname);
}

void IRCserver::deleteUser(int socket) {
    for (auto it = usersAck.begin(); it != usersAck.end(); ++it)
        if (it->second.getSocket() == socket)   {
            usersAck.erase(it);
        }
}

void IRCserver::privateMessageCommand(const Message &msg, const User &usr)  {
    std::string buf;
    std::string cmd = msg.getCommand();

    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "PRIVMSG")
        return;
    if (msg.getParamets().empty())  {
        buf = ":" + serverHostname + " 411 " + usr.getNickname() +
              +" :No recipient given PRIVMSG";
        sendData(usr.getSocket(), buf);
        return;
    }
    if (msg.getParamets().size() == 1)  {
        buf = ":" + serverHostname + " 412 " + usr.getNickname() + " :No text to send";
        sendData(usr.getSocket(), buf);
        return;
    }
    auto userIterator = usersAck.begin();
    auto channelIterator = userChannels.begin();
    for (size_t i = 0; i != msg.getParamets().size() - 1; ++i)  {
        for (size_t j = 0; j != msg.getParamets().size() - 1; ++j)  {
            if (i != j && msg.getParamets()[i] == msg.getParamets()[j]) {
                buf = ":" + serverHostname + " 407 " + usr.getNickname() + " " + msg.getParamets()[i] + " :Duplicate recipients. No message delivered";
                userIterator = usersAck.find(msg.getParamets()[i]);
                sendData(userIterator->second.getSocket(), buf);
                return;
            }
        }
    }
    for (size_t i = 0; i != msg.getParamets().size() - 1; ++i)  {
        userIterator = usersAck.find(msg.getParamets()[i]);
        channelIterator = userChannels.find(msg.getParamets()[i]);
        if (userIterator != usersAck.end())    {
            std::string message(":" + msg.getPrefix() + " PRIVMSG " + userIterator->second.getNickname() + " :" + msg.getParamets().back());
            sendData(userIterator->second.getSocket(), message);
        } else if (channelIterator != userChannels.end()) {
            std::string message(":" + msg.getPrefix() + " PRIVMSG " + channelIterator->second.getName() + " :" + msg.getParamets().back());
            sendDataToChannel(channelIterator->second.getName(), message, msg.getPrefix());
        }
        else    {
            buf = ":" + serverHostname + " 401 " + usr.getNickname() + " " + msg.getParamets()[i] + " :No such nick/channel";
            sendData(usr.getSocket(), buf);
            return;
        }
    }
}

void IRCserver::capCommand(const Message &msg, const User &user)  {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "CAP")
        return;
    if (!msg.getParamets().empty() && msg.getParamets()[0] == "LS") {
        buf = "CAP * LS :";
        sendData(user.getSocket(), buf);
    }
}

void IRCserver::passCommand(const Message &msg, User &user) {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "PASS")
        return;
    if (user.hasPassword())    {
        buf = "462 :You may not reregister";
        sendData(user.getSocket(), buf);
        return;
    }
    if (msg.getParamets().empty())  {
        buf = "461 PASS :Not enough parameters";
        sendData(user.getSocket(), buf);
        return;
    }
    if (msg.getParamets()[0] == serverPassword)
        user.enablePassword();
    else    {
        if (user.getNickname().empty())
            buf = "464 * :Password incorrect";
        else
            buf = "464 " + user.getNickname() + " :Password incorrect";
        sendData(user.getSocket(), buf);
    }
}

 bool IRCserver::isCorrectNickname(const std::string &nick)  {
    std::string specialSymbols = "-[]\\^\'{}";
    if (nick.length() > _NICKNAME_LEN || nick.length() == 0)
        return false;
    if (!std::isalpha(nick[0]))
        return false;
    for (size_t i = 1; i < nick.length(); ++i)  {
        if (!(std::isalnum(nick[i]) || specialSymbols.find(nick[i])))
            return false;
    }
    return (true);
}

void IRCserver::nickCommand(const Message &msg, User **user)  {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);
    if (cmd != "NICK")
        return;
    if (msg.getParamets().empty())  {
        buf = "431 :No nickname given";
        sendData((*user)->getSocket(), buf);
        return;
    }
    if (!isCorrectNickname(msg.getParamets()[0]))  {
        sendData((*user)->getSocket(), "432 " + msg.getParamets()[0] + " :Nickname incorrect");
        return;
    }
    if (usersAck.find(msg.getParamets()[0]) != usersAck.end())  {
        buf = "433 " + msg.getParamets()[0] + " : nickname is already taken";
        sendData((*user)->getSocket(), buf);
        return;
    }
    std::string oldNick((*user)->getNickname());
    std::string newNick(msg.getParamets()[0]);
    User copy(**user);
    copy.setNickname(newNick);
    deleteUser((*user)->getSocket());
    addUser(copy);
    User &newUser = usersAck.find(newNick)->second;
    if (!oldNick.empty())  {
        for (auto & userChannel : userChannels)   {
            Channel &channel = userChannel.second;
            if (channel.RemoveNicknameUsers(oldNick))
                channel.addUsernameVec(newUser);
            if (channel.removeUser(oldNick))    {
                std::cout << "user added???" << std::endl;
                channel.addUser(newUser);
            }
        }
        if (userOpts.erase(oldNick))
            userOpts.insert(std::make_pair(newNick, &newUser));
    }
    if (oldNick.empty())
        buf = "NICK " + newNick;
    else
        buf = ":" + oldNick + " NICK :" + newNick;
    sendDataToJoinedChannels(newNick, buf);
    (*user) = &(usersAck.find(newNick)->second);
    sendData((*user)->getSocket(), buf);
    (*user)->enableNickname();
    if ((*user)->hasNick() && (*user)->isActiveUser() && !(*user)->isLogged())  {
        (*user)->enableLogged();
        buf = "001 " + (*user)->getNickname() + " :Welcome to the Internet Relay Network, " + (*user)->getNickname() + "\r\n";
        buf += "002 " + (*user)->getNickname() + " :Your host is " + serverHostname + ", running version <version>" + "\r\n";
        buf += "003 " + (*user)->getNickname() + " :This server was created <datetime>" + "\r\n";
        buf += "004 " + (*user)->getNickname() + " " + serverHostname + " 1.0/UTF-8 aboOirswx abcehiIklmnoOpqrstvz" + "\r\n";
        sendData((*user)->getSocket(), buf);
    }
}

void IRCserver::userCommand(const Message &msg, User &user)   {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "USER")
        return;
    if (msg.getParamets().size() < 4)   {
        buf = "461 NICK :Not enough parameters";
        sendData(user.getSocket(), buf);
        return;
    }
    if (user.isActiveUser())    {
        buf = "462 :Already registered";
        sendData(user.getSocket(), buf);
        return;
    }
    user.enableUser();
    if (user.hasNick() && user.isActiveUser() && !user.isLogged())  {
        user.enableLogged();
        buf = "001 " + user.getNickname() + " :Welcome to the Internet Relay Network, " + user.getNickname() + "\r\n";
        buf += "002 " + user.getNickname() + " :Your host is " + serverHostname + ", running version <version>" + "\r\n";
        buf += "003 " + user.getNickname() + " :This server was created <datetime>" + "\r\n";
        buf += "004 " + user.getNickname() + " " + serverHostname + " 1.0/UTF-8 aboOirswx abcehiIklmnoOpqrstvz" + "\r\n";
        sendData(user.getSocket(), buf);
    }
}

void IRCserver::pingCommand(const Message &msg, const User &user) {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "PING")
        return;
    buf = "PONG " + serverHostname;
    sendData(user.getSocket(), buf);
}

void IRCserver::noticeCommand(const Message &msg) {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "NOTICE" || msg.getParamets().empty() || msg.getParamets().size() == 1)
        return;
    for (size_t i = 0; i != msg.getParamets().size() - 1; ++i)  {
        for (size_t j = 0; j != msg.getParamets().size() - 1; ++j)  {
            if (i != j && msg.getParamets()[i] == msg.getParamets()[j])
                return;
        }
    }
    auto userIterator = usersAck.begin();
    auto  channelIterator = userChannels.begin();
    for (size_t i = 0; i != msg.getParamets().size() - 1; ++i)  {
        userIterator = usersAck.find(msg.getParamets()[i]);
        channelIterator = userChannels.find(msg.getParamets()[i]);
        if (userIterator != usersAck.end()) {
            std::string message(":" + msg.getPrefix() + " PRIVMSG " + userIterator->second.getNickname() + " :" + msg.getParamets().back());
            sendData(userIterator->second.getSocket(), message);
        }   else if (channelIterator != userChannels.end()) {
            std::string message(":" + msg.getPrefix() + " PRIVMSG " + channelIterator->second.getName() + " :" + msg.getParamets().back());
            sendDataToChannel(channelIterator->first, message, msg.getPrefix());
        }
        else
            return;
    }
}

void IRCserver::joinCommand(const Message &msg, User &usr)    {
    const std::string specialSymbols = " ,\a\r\n";
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "JOIN")
        return;
    std::string dataToSend;
    if (msg.getParamets().empty())  {
        dataToSend = "461 " + usr.getNickname() + " JOIN " + ":Not enough parameters";
        std::cout << usr.getNickname() + " JOIN " + ":Not enough parameters" << std::endl;
        sendData(usr.getSocket(), dataToSend);
        return;
    }
    if (msg.getParamets()[0][0] != '#' && msg.getParamets()[0][0] != '&')   {
        // first param should be group
        dataToSend = "400 " + usr.getNickname() + " JOIN " + ":Could not process invalid parameters";
        std::cout << usr.getNickname() + " JOIN " + ":Could not process invalid parameters" << std::endl;
        sendData(usr.getSocket(), dataToSend);
        return;
    }
    std::vector<std::string> params;
    std::vector<std::string> passwords;
    for (auto tmp_param : msg.getParamets())   {
        // check valid name of a group
        for (auto k : tmp_param)   {
            if (k == 0 || specialSymbols.find(k) != std::string::npos)   {
                dataToSend = "400 " + usr.getNickname() + " JOIN " + ":Could not process invalid parameters";
                std::cout << usr.getNickname() + " JOIN " + ":Could not process invalid parameters" << std::endl;
                sendData(usr.getSocket(), dataToSend);
                return;
            }
        }
        if (tmp_param[0] == '#' || tmp_param[0] == '&')
            params.push_back(tmp_param);
        else
            passwords.push_back(tmp_param);
    }
    if (passwords.size() > params.size())   {
        std::cout << "more passes were provided than groups" << std::endl;
        return;
    }
    for (size_t i = 0; i < params.size(); i++)  {
        std::string tmp_group = params[i];
        std::map<std::string, Channel>::iterator ch_it;
        ch_it = userChannels.find(tmp_group);
        if (ch_it != userChannels.end())    {
            try {
                auto val = passwords.at(i);
                if (!(ch_it->second.getPass() == passwords[i])) {
                    dataToSend = "475 " + usr.getNickname() + " " + ch_it->first + " " + ":Cannot join channel (+k)";
                    std::cout << usr.getNickname() + " " + ch_it->first + " " + ":Cannot join channel (+k)" << std::endl;
                    sendData(usr.getSocket(), dataToSend);
                    return;
                }
            }
            catch (...) {
                if (!ch_it->second.getPass().empty())   {
                    dataToSend = "475 " + usr.getNickname() + " " + ch_it->first + " " + ":Cannot join channel (+k)";
                    std::cout << usr.getNickname() + " " + ch_it->first + " " + ":Cannot join channel (+k)" << std::endl;
                    sendData(usr.getSocket(), dataToSend);
                    return;
                }
            }
            std::map<std::string, User *>::const_iterator user_search_it;
            user_search_it = ch_it->second.getUsers().find(usr.getNickname());
            if (user_search_it != ch_it->second.getUsers().end())   {
                return;
            }
            int num_groups_user_in = 0;
            std::map<std::string, Channel>::iterator max_group_it;
            max_group_it = userChannels.begin();
            for (; max_group_it != userChannels.end(); max_group_it++)  {
                if (max_group_it->second.getUsers().find(usr.getNickname()) != max_group_it->second.getUsers().end())
                    num_groups_user_in++;
            }
            if (num_groups_user_in > 10)    {
                dataToSend = "405 " + usr.getNickname() + " " + ch_it->first + " " + ":You have joined too many channels";
                std::cout << usr.getNickname() + " " + ch_it->first + " " + ":You have joined too many channels" << std::endl;
                sendData(usr.getSocket(), dataToSend);
                return;
            }
            ch_it->second.addUser(usr);
            dataToSend = ":" + usr.getNickname() + " JOIN :" + ch_it->second.getName();
            sendDataToChannel(ch_it->second.getName(), dataToSend);
        }   else    {
            int num_groups_user_in = 0;
            std::map<std::string, Channel>::iterator ban_it;
            ban_it = userChannels.begin();
            for (; ban_it != userChannels.end(); ban_it++)  {
                if (ban_it->second.getUsers().find(usr.getNickname()) != ban_it->second.getUsers().end())
                    num_groups_user_in++;
            }
            if (num_groups_user_in > 10)    {
                dataToSend = "405 " + usr.getNickname() + " " + ch_it->first + " " + ":You have joined too many channels";
                std::cout << usr.getNickname() + " " + ch_it->first + " " + ":You have joined too many channels" << std::endl;
                sendData(usr.getSocket(), dataToSend);
                return;
            }
            if (usr.getNickname().empty())
                return;
            Channel new_ch(tmp_group);
            new_ch.addUser(usr);
            new_ch.addUsernameVec(usr);
            try {
                auto val = passwords.at(i);
                new_ch.setPass(passwords[i]);
            }
            catch (...) {
            }
            userChannels.insert(std::make_pair(new_ch.getName(), new_ch));
            dataToSend = ":" + usr.getNickname() + " JOIN :" + new_ch.getName();
            sendData(usr.getSocket(), dataToSend);
        }
    }
    std::string namesMsg;
    namesMsg = "NAMES ";
    for (auto & param : params)
        namesMsg += param + ",";
    namesMsg.erase(namesMsg.size() - 1);
    _NAMES(Message(namesMsg, usr), usr);
}

void IRCserver::partCommand(const Message &msg, const User &usr)  {
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "PART")
        return;
    std::string dataToSend;
    if (msg.getParamets().empty())  {
        dataToSend = "461 " + usr.getNickname() + " PART " + ":Not enough parameters";
        std::cout << usr.getNickname() + " PART " + ":Not enough parameters" << std::endl;
        sendData(usr.getSocket(), dataToSend);
        return;
    }
    std::vector<std::string> params;
    std::string tmp_param;
    for (const auto & i : msg.getParamets())   {
        if (i[0] != '#' && i[0] != '&') {
            dataToSend = "400 " + usr.getNickname() + " PART " + ":Could not process invalid parameters";
            std::cout << usr.getNickname() + " PART " + ":Could not process invalid parameters" << std::endl;
            sendData(usr.getSocket(), dataToSend);
        }   else    {
            tmp_param = i;
            params.push_back(tmp_param);
        }
    }
    for (auto & param : params)  {
        std::map<std::string, Channel>::iterator ch_it;
        ch_it = userChannels.find(param);
        if (ch_it != userChannels.end())    {
            std::map<std::string, User *>::const_iterator user_search_it;
            user_search_it = ch_it->second.getUsers().find(usr.getNickname());
            if (user_search_it != ch_it->second.getUsers().end())   {
                dataToSend = ":" + usr.getNickname() + " PART :" + ch_it->first;
                sendDataToChannel(ch_it->first, dataToSend);
                ch_it->second.removeUser(usr.getNickname());
                if (ch_it->second.getUsers().empty())
                    userChannels.erase(ch_it->first);
            }   else    {
                dataToSend = "442 " + usr.getNickname() + " " + param + " " + ":You're not on that channel";
                std::cout << usr.getNickname() + " " + param + " " + ":You're not on that channel" << std::endl;
                sendData(usr.getSocket(), dataToSend);
            }
        }   else    {
            dataToSend = "403 " + usr.getNickname() + " " + param + " " + ":No such channel";
            std::cout << usr.getNickname() + " " + param + " " + ":No such channel" << std::endl;
            sendData(usr.getSocket(), dataToSend);
        }
    }
}

void IRCserver::operationCommand(const Message &msg, const User &user) {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "OPER")
        return;
    if (msg.getParamets().size() < 2)
        buf = ":" + serverHostname + " 461 " + user.getNickname() + " OPER :Not enough parameters";
    else if (msg.getParamets()[0] != user.getNickname())
        buf = ":" + serverHostname + " 491 " + user.getNickname() + " :No O-lines for your host";
    else if (msg.getParamets()[1] != serverPassword)
        buf = ":" + serverHostname + " 464 " + user.getNickname() + " :Password incorrect";
    else    {
        auto it = usersAck.find(user.getNickname());
        if (it != usersAck.end())   {
            userOpts.insert(std::make_pair(user.getNickname(), &it->second));
            buf = ":" + serverHostname + " 381 " + user.getNickname() + " :You are now an IRC operator";
        }   else
            std::cerr << RED << "Something went wrong : OPER : No such user" << RESET << std::endl;
    }
    sendData(user.getSocket(), buf);
}

void IRCserver::listCommand(const Message &msg, const User &user)   {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "LIST")
        return;
    buf = ":" + serverHostname + " 321 " + user.getNickname() + " Channel :Users  Name";
    sendData(user.getSocket(), buf);
    if (msg.getParamets().empty())  {
        for (auto & userChannel : userChannels) {
            const Channel &channel = userChannel.second;
            std::stringstream ss;
            ss << channel.getUsers().size();
            buf = ":" + serverHostname + " 322 " + user.getNickname() + " " + channel.getName() + " " + ss.str() + " :" + channel.getTopic();
            sendData(user.getSocket(), buf);
        }
    }   else    {
        for (const auto & i : msg.getParamets())   {
           auto it = userChannels.find(i);
            if (it == userChannels.end())
                continue;
            const Channel &channel = it->second;
            std::stringstream ss;
            ss << channel.getUsers().size();
            buf = ":" + serverHostname + " 322 " + user.getNickname() + " " + channel.getName() + " " + ss.str() + " :" + channel.getTopic();
            sendData(user.getSocket(), buf);
        }
    }
    buf = ":" + serverHostname + " 323 " + user.getNickname() + " :End of /LIST";
    sendData(user.getSocket(), buf);
}

void IRCserver::topicCommand(const Message &msg, const User &user)    {
    std::map<std::string, Channel>::iterator ch_it;
    std::string buf_string;
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);

    if (cmd != "TOPIC")
        return;
    if (msg.getParamets().empty())  {
        buf = ":" + serverHostname + " 461 " + user.getNickname() + " " + "TOPIC :Not enough parameters";
        sendData(user.getSocket(), buf);
        return;
    }
    buf_string = msg.getParamets()[0];
    if (buf_string[0] != '#' && buf_string[0] != '&')   {
        buf = ":" + serverHostname + " 403 " + user.getNickname() + " " + buf_string + " :No such channel";
        sendData(user.getSocket(), buf);
        return;
    }
    ch_it = userChannels.find(buf_string);
    if (ch_it != userChannels.end())    {
        if (msg.getParamets().size() == 2)  {
            ch_it->second.setTopic(msg.getParamets()[1]);
            buf = ":" + user.getNickname() + " TOPIC " + buf_string + " :" + ch_it->second.getTopic();
            sendData(user.getSocket(), buf);
            return;
        }   else if (ch_it->second.getTopic().empty())  {
            buf = ":" + serverHostname + " 331 " + user.getNickname() + " " + buf_string + " :No topic is set";
            sendData(user.getSocket(), buf);
            return;
        }   else if (!ch_it->second.getTopic().empty()) {
            buf = ":" + serverHostname + " 332 " + user.getNickname() + " " + buf_string + " :" + ch_it->second.getTopic();
            sendData(user.getSocket(), buf);
        }
    }   else    {
        buf = ":" + serverHostname + " 403 " + user.getNickname() + " " + buf_string + " :No such channel";
        sendData(user.getSocket(), buf);
        return;
    }
}

// TODO: change me
void IRCserver::_NAMES(const Message &msg, const User &user)    {
    (void)msg;
    (void)user;
//    std::map<std::string, Channel>::iterator ch_it;
//    std::string message;
//    std::string buf;
//    std::vector<std::string> buf_string;
//    std::string cmd = msg.getCommand();
//    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);
//
//    if (cmd != "NAMES")
//        return;
//    for (size_t i = 0; i < msg.getParamets().size(); ++i)   {
//        buf_string.push_back(msg.getParamets()[i]);
//        if (!(!buf_string.empty() && (buf_string[i][0] == '#' || buf_string[i][0] == '&')))
//            buf_string.pop_back();
//    }
//   auto channelIter = userChannels.begin();
//    if (!buf_string.empty())    {
//        for (auto i = 0; i < buf_string.size(); ++i)  {
//            std::map<std::string, User *>::const_iterator ch_us_it;
//            std::vector<User>::const_iterator chusernameVec_it;
//            ch_it = userChannels.find(buf_string[i]);
//            if (ch_it != userChannels.end())    {
//                ch_us_it = ch_it->second.getUsers().begin();
//                message = ":" + serverHostname + " 353 " + user.getNickname() + " = " + ch_it->second.getName() + " :";
//                for (; ch_us_it != ch_it->second.getUsers().end(); ++ch_us_it)  {
//                    chusernameVec_it = ch_it->second.getUsersByNickname(ch_us_it->second->getNickname());
//                    if (chusernameVec_it != ch_it->second.getUsersByNicknames().end())
//                        buf += "@" + ch_us_it->second->getNickname() + " ";
//                    else
//                        buf += ch_us_it->second->getNickname() + " ";
//                }
//                sendData(user.getSocket(), message + buf);
//                message = ":" + serverHostname + " 366 " + user.getNickname() + " " + ch_it->second.getName() + " :End of /NAMES list";
//                sendData(user.getSocket(), message);
//            }
//            else    {
//                message = ":" + serverHostname + " 366 " + user.getNickname() + " " + buf_string[i] + " :End of /NAMES list";
//                sendData(user.getSocket(), message);
//            }
//            buf.clear();
//        }
//    }
}

void IRCserver::inviteCommand(const Message &msg, const User &user)   {
    std::map<std::string, Channel>::iterator ch_it;
    std::map<std::string, User *>::const_iterator us_ch_it;
    std::string buf;
    std::string bufChan;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);
    if (cmd != "INVITE")
        return;
    if (msg.getParamets().size() < 2)   {
        buf = ":" + serverHostname + " 461 " + user.getNickname() + " " + "Not enough parameters sent for invite";
        sendData(user.getSocket(), buf);
        return;
    }
    bufChan = msg.getParamets()[1];
    if (bufChan[0] != '#' && bufChan[0] != '&')
        return;
    auto userIterator = usersAck.find(msg.getParamets()[0]);
    if (userIterator == this->usersAck.end() || userChannels.empty())   {
        buf = ":" + serverHostname + " 401 " + user.getNickname() + " " + msg.getParamets()[0] + " :No such nick or channel";
        sendData(user.getSocket(), buf);
        return;
    }
    auto userInChanel = ch_it->second.getUsers().find(user.getNickname());
    if (userInChanel != ch_it->second.getUsers().end()) {
        userInChanel = ch_it->second.getUsers().find(msg.getParamets()[0]);
        if (userInChanel != ch_it->second.getUsers().end()) {
            buf = ":" + serverHostname + " 443 " + user.getNickname() + " " + msg.getParamets()[0] + " :is already on channel" + bufChan;
            sendData(user.getSocket(), buf);
        }   else    {
            buf = ":" + serverHostname + " 341 " + user.getNickname() + " " + msg.getParamets()[0] + " " + bufChan;
            sendData(user.getSocket(), buf);
            buf = ":" + user.getNickname() + " INVITE " + msg.getParamets()[0] + " :" + bufChan;
            sendData(userInChanel->second->getSocket(), buf);
        }
    }   else    {
        buf = ":" + serverHostname + " 442 " + user.getNickname() + " #" + bufChan + " :You're not on that channel";
        sendData(user.getSocket(), buf);
        return;
    }
}

void IRCserver::quitCommand(const Message &msg, User **user) {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);
    if (cmd != "QUIT")
        return;
    if (msg.getParamets().empty())
        buf = ":" + (*user)->getNickname() + " Quit: " + (*user)->getNickname();
    else
        buf = ":" + (*user)->getNickname() + " Quit: " + msg.getParamets()[0];
    for (auto & userChannel : userChannels)  {
        auto userIt = userChannel.second.getUsers().find((*user)->getNickname());
        if (userIt != userChannel.second.getUsers().end())
            sendDataToChannel(userChannel.second.getName(), buf, (*user)->getNickname());
    }
    buf = "Error, closing connection";
    sendData((*user)->getSocket(), buf);
    // removing the user
    close((*user)->getSocket());
    FD_CLR((*user)->getSocket(), &userFdSet);

    deleteUser((*user)->getNickname());

    *user = nullptr;
}

void IRCserver::killCommand(const Message &msg, User **user)  {
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);
    if (cmd != "KILL")
        return;
    if (msg.getParamets().size() < 2)   {
        buf = ":" + serverHostname + " 461 KILL :Not enough parameters";
        sendData((*user)->getSocket(), buf);
        return;
    }
    if (userOpts.find((*user)->getNickname()) == userOpts.end())    {
        buf = ":" + serverHostname + " 481 " + (*user)->getNickname() + " :Permission Denied- You're not an IRC operator";
        sendData((*user)->getSocket(), buf);
        return;
    }
    if (usersAck.find(msg.getParamets()[0]) == usersAck.end())  {
        buf = ":" + serverHostname + " 401 " + msg.getParamets()[0] + " :No such nick";
        sendData((*user)->getSocket(), buf);
        return;
    }
    buf = ":" + (*user)->getNickname() + " KILL " + msg.getParamets()[0] + " :" + msg.getParamets()[1];
    sendData((*user)->getSocket(), buf);
    buf = ":localhost QUIT :Killed (" + (*user)->getNickname() + " (" + msg.getParamets()[1] + "))";
    // sending quit message to all
    User *killedUser = &usersAck.find(msg.getParamets()[0])->second;
    User tmp;
    tmp.setNickname(serverHostname);
    quitCommand(Message(buf, tmp), &killedUser);
}

// TODO: change me
void IRCserver::_KICK(const Message &msg, const User &user) {
    (void)msg;
    (void)user;
//    std::string buf;
//    std::string cmd = msg.getCommand();
//    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);
//    if (cmd != "KICK")
//        return;
//    if (msg.getParamets().size() < 2)   {
//        buf = ":" + serverHostname + " 461 KILL :Not enough parameters";
//        sendData(user.getSocket(), buf);
//        return;
//    }
//    if (userChannels.find(msg.getParamets()[0]) == userChannels.end())  {
//        buf = ":" + serverHostname + " 403 " + user.getNickname() + " " + msg.getParamets()[0] + " :No such channel";
//        sendData(user.getSocket(), buf);
//        return;
//    }
//    Channel &channel = userChannels.find(msg.getParamets()[0])->second;
//    if (channel. .find(user.getNickname()) == channel.getUsers().end())    {
//        buf = ":" + serverHostname + " 442 " + channel.getName() + " :You're not on that channel";
//        sendData(user.getSocket(), buf);
//        return;
//    }
//    if (channel.getUsersByNickname(user.getNickname()) == channel.getUsersByNicknames().end())  {
//        buf = ":" + serverHostname + " 482 " + user.getNickname() + " " + channel.getName() + " :You're not channel operator";
//        sendData(user.getSocket(), buf);
//        return;
//    }
//    if (channel.getUsers().find(msg.getParamets()[1]) == channel.getUsers().end())  {
//        buf = ":" + serverHostname + " 441 " + msg.getParamets()[1] + " " + channel.getName() + " :They aren't on that channel";
//        sendData(user.getSocket(), buf);
//        return;
//    }
//    buf = ":" + user.getNickname() + " KICK " + msg.getParamets()[0] + " " + msg.getParamets()[1];
//    _send(channel.getUsers().find(msg.getParamets()[1])->second->getSocket(), buf);
//    // removing user
//    channel.removeUser(msg.getParamets()[1]);
//    // removing channel
//    if (channel.getUsers().size() == 0)
//    {
//        userChannels.erase(channel.getName());
//        return;
//    }
//    sendDataToChannel(msg.getParamets()[0], buf, msg.getParamets()[1]);
}



void IRCserver::executeCommand(int socket, const std::string &bufferExec) {
    auto userIt = usersAck.begin();
    for (; userIt != usersAck.end(); ++userIt)    {
        if (userIt->second.getSocket() == socket)
            break ;
    }
    if (userIt == usersAck.end())
        return;
    Message msg(bufferExec, userIt->second);
    User *user = &userIt->second;
    if (serverPassword.empty() && !user->hasPassword())
        user->enablePassword();
    if (user->hasPassword() && user->isLogged())    {
        quitCommand(msg, &user);
        if (user == nullptr)
            return;
        killCommand(msg, &user);
        if (user == nullptr)
            return;
    }
    capCommand(msg, *user);
    passCommand(msg, *user);
    pingCommand(msg, *user);
    if (user->hasPassword())   {
        nickCommand(msg, &user);
        userCommand(msg, *user);
    }
    if (user->hasPassword() && user->isLogged())   {
        privateMessageCommand(msg, *user);
        noticeCommand(msg);
        joinCommand(msg, *user);
        partCommand(msg, *user);
        listCommand(msg, *user);
        operationCommand(msg, *user);
        _KICK(msg, *user);
        _NAMES(msg, *user);
        topicCommand(msg, *user);
        inviteCommand(msg, *user);
    }
}

void IRCserver::sendDataToJoinedChannels(const std::string &nick, const std::string &buf) {
    (void)nick;
    (void)buf;
}

void IRCserver::sendDataToChannel(const std::string &channel, const std::string &buf, const std::string &nick) {
    (void)nick;
    (void)buf;
    (void)channel;
}

bool IRCserver::receiveData(int socket, std::string &buf) {
    char c_buf[512];
    int bytesLeft;
    int bytes = 1;
    int res;

    if (getUserIter(socket) == usersAck.end())
        return (false);
    User &user = getUserIter(socket)->second;
    buf.clear();
    while (buf.find(dataDelimeter) == std::string::npos && user.getBuffer().size() + buf.size() < sizeof(c_buf))
    {
        memset(c_buf, 0, sizeof(c_buf));
        bytes = int(recv(socket, c_buf, sizeof(c_buf) - 1 - (user.getBuffer().size() + buf.size()), MSG_PEEK));
        if (bytes < 0)  {
            if (errno == EAGAIN)    {
                user.bufferAppend(buf);
                return (false);
            }
            std::cerr << RED << strerror(errno) << RESET;
            throw std::exception();
        }
        if (!bytes)
            throw std::exception();
        bytesLeft = int(std::string(c_buf).find(dataDelimeter));
        if (bytesLeft == -1)
            bytesLeft = bytes;
        else
            bytesLeft += int(dataDelimeter.length());
        while (bytesLeft > 0)   {
            bzero((void*)c_buf, sizeof(c_buf));
            bytes = int(recv(socket, c_buf, static_cast<size_t>(bytesLeft), 0));
            if (bytes < 0)  {
                if (errno == EAGAIN)    {
                    user.bufferAppend(buf);
                    return (false);
                }
                std::cerr << RED << strerror(errno) << RESET;
                throw std::exception();
            }
            if (bytes == 0)
                throw std::exception();
            bytesLeft -= bytes;
            buf += c_buf;
        }
    }
    if (buf.find(dataDelimeter) == std::string::npos)
        res = false;
    else
        res = true;
    user.bufferAppend(buf);
    buf = user.getBuffer();
    user.bufferClear();
    buf.erase(buf.end() - (long)dataDelimeter.length(), buf.end());
    std::cout << CYAN << "▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽" << '\n';
    std::cout  << "-----------RECIEVED-----------" << '\n';
    std::cout  << "socket  : " << socket << '\n';
    std::cout  << "msg     : "  << buf << '\n';
    std::cout  << "△△△△△△△△△△△△△△△△△△△△△△△△△△△△△△" << RESET << std::endl;
    return (res);
}

bool IRCserver::sendData(int socket, const std::string &buf) {
        std::string buf_delim(buf);
        int total = 0;
        int bytesLeft;
        int bytes;

        if (getUserIter(socket) == usersAck.end())
            return (false);
        User &user = getUserIter(socket)->second;
        std::cout << GREEN << "▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼" << '\n';
        std::cout <<  "------------SENDED------------" << '\n';
        std::cout << "socket  : "  << socket << '\n';
        std::cout <<  "msg     : "  << buf << '\n';
        std::cout  << "▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲" << RESET << std::endl;
        if (buf_delim.find(dataDelimeter) != buf_delim.length() - dataDelimeter.length())
            buf_delim += dataDelimeter;
        bytesLeft = int(buf_delim.length());
        while (bytesLeft > 0)   {
            bytes = int(send(socket, buf_delim.c_str() + total, static_cast<size_t>(bytesLeft), 0));
            if (bytes < 0)  {
                if (errno == EAGAIN)    {
                    user.setUserSendBuffer(buf_delim.c_str() + total);
                    return (false);
                }
                std::cerr << RED << strerror(errno) << RESET;
                break;
            }
            total += bytes;
            bytesLeft -= bytes;
        }
        return bytes != -1;
}

sockaddr_in IRCserver::getServerAddress() const {
    return serverAddress;
}
