#include "../includes/IRCserver.hpp"
#include "../includes/utils.hpp"
#include "../includes/User.hpp"
#include "../includes/Message.hpp"


static bool exitFlag = false;   // TODO: rename

void    sigintCatcher(int sig)  // TODO: rename
{
    if(sig == SIGINT)
        exitFlag = true;
}

IRCserver::IRCserver(uint32_t port, std::string password):
    IRCserverInterface(port, password)
{
    dataDelimeter = _DELIM;
    setHostname();
    setServerAdress();
    signal(SIGPIPE, SIG_IGN);
//    signal(SIGINT, sigintCatcher); // TODO: sigintCatcher
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
void IRCserver::setListener(int32_t listener) {
    listener = listener;
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
void IRCserver::setbuffer(const std::string &buffer) {
    buffer = buffer;
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
    return _hostname;
}
void IRCserver::setHostname() {
    char hostname[_HOSTNAME_LEN];
    bzero((void*)(hostname), _HOSTNAME_LEN);
    if(gethostname(hostname, _HOSTNAME_LEN) != -1)
        _hostname = hostname;
}

sockaddr_in  IRCserver::getServerAdress() const {
    return _serverAdress;
}

void IRCserver::setServerAdress() {
    _serverAdress.sin_family = AF_INET;
    _serverAdress.sin_addr.s_addr = INADDR_ANY;
    _serverAdress.sin_port = htons(_port);
}

IRCserver::IRCserver() {}

void IRCserver::initListener() {

    FD_ZERO(&userFdSet);
    listener = socket(AF_INET, SOCK_STREAM, getprotobyname("TCP")->p_proto);
    if(listener < 0)
        throw std::invalid_argument(strerror(errno));
    fdLimit = listener;

    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    int b = bind(listener,(struct sockaddr *)&_serverAdress, sizeof(_serverAdress));
    if(b < 0)
        throw std::invalid_argument(strerror(errno));

    int l = listen(listener, 10);
    if(l < 0)
        throw std::invalid_argument(strerror(errno));

    FD_SET(listener, &userFdSet);
    fdLimit = listener;
};

IRCserver::~IRCserver() = default;


void IRCserver::acceptConnection()
{
    struct sockaddr_in temp{};

    socklen_t socklen = sizeof(struct sockaddr_in);
    int new_fd = accept(listener,(struct sockaddr *)&temp, &socklen);
    if(new_fd == -1)
        throw std::invalid_argument(strerror(errno));
    fcntl(listener, F_SETFL, O_NONBLOCK);
    addUser(new_fd);
    if(fdLimit < new_fd)
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
            if (usrIterator == usersAck.end())    {
                continue;
            }
            if (usrIterator->second.getSendbuffer().length())
                sendData(i, usrIterator->second.getSendbuffer());
            try {
                recieveData(i, localBuffer);
            } catch (const std::exception &e)   {
                auto msg = Message("QUIT :Remote host closed the connection", usrIterator->second);
                quitCommad(msg, reinterpret_cast<User **>(&usrIterator->second));
            }
            _execute(i, buffer);
            std::cout << "Number of users :\t "
                      << usersAck.size() << std::endl;
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


void    IRCserver::serverShutdown()
{
    std::unordered_multimap<std::string, User>::iterator    it;

    for (it = usersAck.begin(); it != usersAck.end(); ++it)
    {
        Message msg = Message("Stopped: ", it->second);
        quitCommad(msg, reinterpret_cast<User **>(&it->second));
    }
    FD_ZERO(&userFdSet);
}

std::unordered_multimap<std::string, User>::iterator IRCserver::getUserIter(int socket) {

    for (std::unordered_multimap<std::string, User>::iterator it = usersAck.begin(); it != usersAck.end(); ++it)  {
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

void IRCserver::privateMessageCommand(const Message &msg, const User &usr)
{
    std::multimap<std::string, User>::iterator us_it;
    std::map<std::string, Channel>::iterator ch_it;
    std::string buffer;

    if (utils::toUpper(msg.getCommand()) != "PRIVMSG")
        return;
    if (msg.getParamets().empty())  {
        buffer = ":" + _hostname + " 411 " + usr.getNickname() +
              +" :No recipient given PRIVMSG";
        sendData(usr.getSocket(), buffer);
        return;
    }
    if (msg.getParamets().size() == 1)  {
        buffer = ":" + _hostname + " 412 " + usr.getNickname() + " :No text to send";
        sendData(usr.getSocket(), buffer);
        return;
    }
    for (size_t i = 0; i != msg.getParamets().size() - 1; ++i)  {
        for (size_t j = 0; j != msg.getParamets().size() - 1; ++j)  {
            if (i != j && msg.getParamets()[i] == msg.getParamets()[j]) {
                buffer = ":" + _hostname + " 407 " + usr.getNickname() + " " + msg.getParamets()[i] + " :Duplicate recipients. No message delivered";
                us_it = usersAck.find(msg.getParamets()[i]);
                sendData(us_it->second.getSocket(), buffer);
                return;
            }
        }
    }
    us_it = usersAck.begin();
    ch_it = userChannels.begin();
    for (size_t i = 0; i != msg.getParamets().size() - 1; ++i)  {
        us_it = usersAck.find(msg.getParamets()[i]);
        ch_it = userChannels.find(msg.getParamets()[i]);
        if (us_it != usersAck.end())    {
            std::string message(":" + msg.getPrefix() + " PRIVMSG " + us_it->second.getNickname() + " :" + msg.getParamets().back());

            sendData(us_it->second.getSocket(), message);
        } else if (ch_it != userChannels.end()) {
            std::string message(":" + msg.getPrefix() + " PRIVMSG " + ch_it->second.getName() + " :" + msg.getParamets().back());
            sendDataToChannel(ch_it->second.getName(), message, msg.getPrefix());
        }
        else    {
            buffer = ":" + _hostname + " 401 " + usr.getNickname() + " " + msg.getParamets()[i] + " :No such nick/channel";
            sendData(usr.getSocket(), buffer);
            return;
        }
    }
}

void IRCserver::_CAP(const Message &msg, const User &user)  {
    std::string buffer;

    if (utils::toUpper(msg.getCommand()) != "CAP")
        return;
    if (msg.getParamets().size() > 0 && msg.getParamets()[0] == "LS") {
        buffer = "CAP * LS :";
        sendData(user.getSocket(), buffer);
    }
}

void IRCserver::_PASS(const Message &msg, User &user) {
    std::string buffer;
    if (utils::toUpper(msg.getCommand()) != "PASS")
        return;
    if (user.hasPassworded())    {
        buffer = "462 :You may not reregister";
        sendData(user.getSocket(), buffer);
        return;
    }
    if (msg.getParamets().size() == 0)  {
        buffer = "461 PASS :Not enough parameters";
        sendData(user.getSocket(), buffer);
        return;
    }
    if (msg.getParamets()[0] == _password)
        user.unablePassword();
    else
    {
        if (user.getNickname().empty())
            buffer = "464 * :Password incorrect";
        else
            buffer = "464 " + user.getNickname() + " :Password incorrect";
        sendData(user.getSocket(), buffer);
    }
}

bool IRCserver::isCorrectNickname(const std::string &nick)
{
    std::string specialSymbols = "-[]\\^\'{}";
    if (nick.length() > _NICKNAME_LEN || nick.length() == 0)
        return false;
    if (!((nick[0] >= 'a' && nick[0] <= 'z') || (nick[0] >= 'A' && nick[0] <= 'Z')))
        return false;
    for (auto i = 1; i < nick.length(); ++i)  {
        if (!(std::isalnum(nick[i]) || specialSymbols.find(nick[i])))
            return false;
    }
    return (true);
}

void IRCserver::_NICK(const Message &msg, User **user)
{
    std::string buffer;
    std::map<std::string, Channel>::iterator chit;
    if (utils::toUpper(msg.getCommand()) != "NICK")
        return;
    if (msg.getParamets().size() == 0)  {
        buffer = "431 :No nickname given";
        sendData((*user)->getSocket(), buffer);
        return;
    }
    if (!isCorrectNickname(msg.getParamets()[0]))  {
        sendData((*user)->getSocket(), "432 " + msg.getParamets()[0] + " :Nickname incorrect");
        return;
    }
    if (usersAck.find(msg.getParamets()[0]) != usersAck.end())  {
        buffer = "433 " + msg.getParamets()[0] + " : nickname is already taken";
        sendData((*user)->getSocket(), buffer);
        return;
    }
    std::string oldNick((*user)->getNickname());
    std::string newNick(msg.getParamets()[0]);
    User copy(**user);

    copy.setNickname(newNick);
    deleteUser((*user)->getSocket());
    addUser(copy);
    User &newUser = usersAck.find(newNick)->second;

    if (oldNick != "")  {
        // removing from channels
        for (chit = userChannels.begin(); chit != userChannels.end(); ++chit)
        {
            Channel &channel = chit->second;

            // is usernameVec?
            if (channel.RemoveNicknameUsers(oldNick))
                channel.addusernameVec(newUser);
            // is user
            if (channel.removeUser(oldNick))
            {
                std::cout << "user added???" << std::endl;
                channel.addUser(newUser);
            }
        }
        // is server operator
        if (userOpts.erase(oldNick))
            userOpts.insert(std::make_pair(newNick, &newUser));
    }

    if (oldNick.empty())
        buffer = "NICK " + newNick;
    else
        buffer = ":" + oldNick + " NICK :" + newNick;
    sendDataToJoinedChannels(newNick, buffer);
    (*user) = &(usersAck.find(newNick)->second);
    sendData((*user)->getSocket(), buffer);
    (*user)->unableNick();
    if ((*user)->hasNick() && (*user)->isActiveUser() && !(*user)->isLogged())
    {
        (*user)->unableLogged();
        buffer = "001 " + (*user)->getNickname() + " :Welcome to the Internet Relay Network, " + (*user)->getNickname() + "\r\n";
        buffer += "002 " + (*user)->getNickname() + " :Your host is " + _hostname + ", running version <version>" + "\r\n";
        buffer += "003 " + (*user)->getNickname() + " :This server was created <datetime>" + "\r\n";
        buffer += "004 " + (*user)->getNickname() + " " + _hostname + " 1.0/UTF-8 aboOirswx abcehiIklmnoOpqrstvz" + "\r\n";
        buffer += "005 " + (*user)->getNickname() + " PREFIX=(ohv)@\%+ CODEPAGES MODES=3 CHANTYPES=#&!+ MAXCHANNELS=20 \
                NICKLEN=31 TOPICLEN=255 KICKLEN=255 NETWORK=school21 \
                CHANMODES=beI,k,l,acimnpqrstz :are supported by this server";
        sendData((*user)->getSocket(), buffer);
    }
}

void IRCserver::_USER(const Message &msg, User &user)
{
    std::string buffer;

    if (utils::toUpper(msg.getCommand()) != "USER")
        return;
    if (msg.getParamets().size() < 4)
    {
        buffer = "461 NICK :Not enough parameters";
        sendData(user.getSocket(), buf);
        return;
    }
    if (user.isActiveUser())
    {
        buf = "462 :Already registered";
        sendData(user.getSocket(), buf);
        return;
    }
    user.unableUser();
    if (user.hasNick() && user.isActiveUser() && !user.isLogged())
    {
        user.unableLogged();
        buf = "001 " + user.getNickname() + " :Welcome to the Internet Relay Network, " + user.getNickname() + "\r\n";
        buf += "002 " + user.getNickname() + " :Your host is " + _hostname + ", running version <version>" + "\r\n";
        buf += "003 " + user.getNickname() + " :This server was created <datetime>" + "\r\n";
        buf += "004 " + user.getNickname() + " " + _hostname + " 1.0/UTF-8 aboOirswx abcehiIklmnoOpqrstvz" + "\r\n";
        buf += "005 " + user.getNickname() + " PREFIX=(ohv)@\%+ CODEPAGES MODES=3 CHANTYPES=#&!+ MAXCHANNELS=20 \
                NICKLEN=31 TOPICLEN=255 KICKLEN=255 NETWORK=school21 \
                CHANMODES=beI,k,l,acimnpqrstz :are supported by this server";
        sendData(user.getSocket(), buf);
    }
}

void IRCserver::_PING(const Message &msg, const User &user)
{
    std::string buf;

    if (utils::toUpper(msg.getCommand()) != "PING")
        return;
    buf = "PONG " + _hostname;
    sendData(user.getSocket(), buf);
}

void IRCserver::_NOTICE(const Message &msg)
{
    std::multimap<std::string, User>::iterator us_it;
    std::map<std::string, Channel>::iterator ch_it;
    std::string buf;

    if (utils::toUpper(msg.getCommand()) != "NOTICE")
        return;
    if (msg.getParamets().empty())
    {
        return;
    }
    if (msg.getParamets().size() == 1)
    {
        return;
    }
    for (size_t i = 0; i != msg.getParamets().size() - 1; ++i)
    {
        for (size_t j = 0; j != msg.getParamets().size() - 1; ++j)
        {
            if (i != j && msg.getParamets()[i] == msg.getParamets()[j])
                return;
        }
    }
    us_it = usersAck.begin();
    ch_it = userChannels.begin();
    for (size_t i = 0; i != msg.getParamets().size() - 1; ++i)
    {
        us_it = usersAck.find(msg.getParamets()[i]);
        ch_it = userChannels.find(msg.getParamets()[i]);
        if (us_it != usersAck.end())
        {
            std::string message(":" + msg.getPrefix() + " PRIVMSG " + us_it->second.getNickname() + " :" + msg.getParamets().back());

            sendData(us_it->second.getSocket(), message);
        }
        else if (ch_it != userChannels.end())
        {
            std::string message(":" + msg.getPrefix() + " PRIVMSG " + ch_it->second.getName() + " :" + msg.getParamets().back());

            sendDataToChannel(ch_it->first, message, msg.getPrefix());
        }
        else
            return;
    }
}

void IRCserver::_JOIN(const Message &msg, User &usr)
{

    if (utils::toUpper(msg.getCommand()) != "JOIN")
        return;

    std::string tosendData;

    if (msg.getParamets().empty())
    {
        tosendData = "461 " + usr.getNickname() + " JOIN " + ":Not enough parameters";
        std::cout << usr.getNickname() + " JOIN " + ":Not enough parameters" << std::endl;
        sendData(usr.getSocket(), tosendData);
        return;
    }

    if (msg.getParamets()[0][0] != '#' && msg.getParamets()[0][0] != '&')
    {
        // first param should be group
        tosendData = "400 " + usr.getNickname() + " JOIN " + ":Could not process invalid parameters";
        std::cout << usr.getNickname() + " JOIN " + ":Could not process invalid parameters" << std::endl;
        sendData(usr.getSocket(), tosendData);
        return;
    }

    std::vector<std::string> params;
    std::vector<std::string> passwords;
    for (size_t i = 0; i < msg.getParamets().size(); i++)
    {

        // check valid name of a group
        std::string tmp_param = msg.getParamets()[i];
        for (size_t k = 0; k < tmp_param.size(); k++)
        {
            if (tmp_param[k] == ' ' ||
                tmp_param[k] == ',' ||
                tmp_param[k] == '\a' ||
                tmp_param[k] == '\0' ||
                tmp_param[k] == '\r' ||
                tmp_param[k] == '\n')
            {

                tosendData = "400 " + usr.getNickname() + " JOIN " + ":Could not process invalid parameters";
                std::cout << usr.getNickname() + " JOIN " + ":Could not process invalid parameters" << std::endl;
                sendData(usr.getSocket(), tosendData);
                return;
            }
        }

        if (tmp_param[0] == '#' || tmp_param[0] == '&')
            params.push_back(tmp_param);
        else
            passwords.push_back(tmp_param);
    }

    if (passwords.size() > params.size())
    {
        std::cout << "more passes were provided than groups" << std::endl;
        return;
    }

    for (size_t i = 0; i < params.size(); i++)
    {

        std::string tmp_group = params[i];
        std::map<std::string, Channel>::iterator ch_it;
        ch_it = userChannels.find(tmp_group);

        if (ch_it != userChannels.end())
        {

            try
            {
                passwords.at(i);
                // invalid pass
                if (!(ch_it->second.getPass() == passwords[i]))
                {

                    tosendData = "475 " + usr.getNickname() + " " + ch_it->first + " " + ":Cannot join channel (+k)";
                    std::cout << usr.getNickname() + " " + ch_it->first + " " + ":Cannot join channel (+k)" << std::endl;
                    sendData(usr.getSocket(), tosendData);
                    return;
                }
            }
            catch (...)
            {
                if (!ch_it->second.getPass().empty())
                {
                    tosendData = "475 " + usr.getNickname() + " " + ch_it->first + " " + ":Cannot join channel (+k)";
                    std::cout << usr.getNickname() + " " + ch_it->first + " " + ":Cannot join channel (+k)" << std::endl;
                    sendData(usr.getSocket(), tosendData);
                    return;
                }
            }

            std::map<std::string, User *>::const_iterator user_search_it;
            user_search_it = ch_it->second.getUsers().find(usr.getNickname());
            if (user_search_it != ch_it->second.getUsers().end())
            {
                // user already in this group;
                return;
            }

            // if more than 10 groups
            int num_groups_user_in = 0;
            std::map<std::string, Channel>::iterator max_group_it;
            max_group_it = userChannels.begin();
            for (; max_group_it != userChannels.end(); max_group_it++)
            {
                if (max_group_it->second.getUsers().find(usr.getNickname()) != max_group_it->second.getUsers().end())
                    num_groups_user_in++;
            }
            if (num_groups_user_in > 10)
            {
                tosendData = "405 " + usr.getNickname() + " " + ch_it->first + " " + ":You have joined too many channels";
                std::cout << usr.getNickname() + " " + ch_it->first + " " + ":You have joined too many channels" << std::endl;
                sendData(usr.getSocket(), tosendData);
                return;
            }

            ch_it->second.addUser(usr);

            tosendData = ":" + usr.getNickname() + " JOIN :" + ch_it->second.getName();

            sendDataToChannel(ch_it->second.getName(), tosendData);
        }
        else
        {
            // if more than 10 groups
            int num_groups_user_in = 0;
            std::map<std::string, Channel>::iterator ban_it;
            ban_it = userChannels.begin();
            for (; ban_it != userChannels.end(); ban_it++)
            {
                if (ban_it->second.getUsers().find(usr.getNickname()) != ban_it->second.getUsers().end())
                    num_groups_user_in++;
            }
            if (num_groups_user_in > 10)
            {
                tosendData = "405 " + usr.getNickname() + " " + ch_it->first + " " + ":You have joined too many channels";
                std::cout << usr.getNickname() + " " + ch_it->first + " " + ":You have joined too many channels" << std::endl;
                sendData(usr.getSocket(), tosendData);
                return;
            }

            // if no nickname
            if (usr.getNickname() == "")
                return;

            Channel new_ch(tmp_group);
            new_ch.addUser(usr);
            new_ch.addusernameVec(usr);

            try
            {
                passwords.at(i);
                new_ch.setPass(passwords[i]);
            }
            catch (...)
            {
            }

            userChannels.insert(std::make_pair(new_ch.getName(), new_ch));

            tosendData = ":" + usr.getNickname() + " JOIN :" + new_ch.getName();
            sendData(usr.getSocket(), tosendData);
        }
    }
    std::string namesMsg;
    namesMsg = "NAMES ";
    for (size_t i = 0; i < params.size(); i++)
        namesMsg += params[i] + ",";

    namesMsg.erase(namesMsg.size() - 1);
    _NAMES(Message(namesMsg, usr), usr);
}

void IRCserver::_PART(const Message &msg, const User &usr)
{

    if (utils::toUpper(msg.getCommand()) != "PART")
        return;

    std::string tosendData;

    if (msg.getParamets().empty())
    {
        tosendData = "461 " + usr.getNickname() + " PART " + ":Not enough parameters";
        std::cout << usr.getNickname() + " PART " + ":Not enough parameters" << std::endl;
        sendData(usr.getSocket(), tosendData);
        return;
    }

    std::vector<std::string> params;
    std::string tmp_param;

    for (size_t i = 0; i < msg.getParamets().size(); i++)
    {
        if (msg.getParamets()[i][0] != '#' && msg.getParamets()[i][0] != '&')
        {
            tosendData = "400 " + usr.getNickname() + " PART " + ":Could not process invalid parameters";
            std::cout << usr.getNickname() + " PART " + ":Could not process invalid parameters" << std::endl;
            sendData(usr.getSocket(), tosendData);
        }
        else
        {
            tmp_param = msg.getParamets()[i];
            params.push_back(tmp_param);
        }
    }

    for (size_t i = 0; i < params.size(); i++)
    {

        std::map<std::string, Channel>::iterator ch_it;
        ch_it = userChannels.find(params[i]);
        if (ch_it != userChannels.end())
        { // channel exists

            std::map<std::string, User *>::const_iterator user_search_it;
            user_search_it = ch_it->second.getUsers().find(usr.getNickname());

            // user in group. should be deleted
            if (user_search_it != ch_it->second.getUsers().end())
            {

                tosendData = ":" + usr.getNickname() + " PART :" + ch_it->first;

                sendDataToChannel(ch_it->first, tosendData);
                ch_it->second.removeUser(usr.getNickname());

                // if group is empty - del it
                if (ch_it->second.getUsers().empty())
                    userChannels.erase(ch_it->first);
            }
            else
            { // no this user in group
                tosendData = "442 " + usr.getNickname() + " " + params[i] + " " + ":You're not on that channel";
                std::cout << usr.getNickname() + " " + params[i] + " " + ":You're not on that channel" << std::endl;
                sendData(usr.getSocket(), tosendData);
            }
        }
        else
        { // channel dosnt exist
            tosendData = "403 " + usr.getNickname() + " " + params[i] + " " + ":No such channel";
            std::cout << usr.getNickname() + " " + params[i] + " " + ":No such channel" << std::endl;
            sendData(usr.getSocket(), tosendData);
        }
    }
}

void IRCserver::_OPER(const Message &msg, const User &user)
{
    std::string buf;
    std::multimap<std::string, User>::iterator it;

    if (utils::toUpper(msg.getCommand()) != "OPER")
        return;
    if (msg.getParamets().size() < 2)
        buf = ":" + _hostname + " 461 " + user.getNickname() + " OPER :Not enough parameters";
    else if (msg.getParamets()[0] != user.getNickname())
        buf = ":" + _hostname + " 491 " + user.getNickname() + " :No O-lines for your host";
    else if (msg.getParamets()[1] != _password)
        buf = ":" + _hostname + " 464 " + user.getNickname() + " :Password incorrect";
    else
    {
        it = usersAck.find(user.getNickname());
        if (it != usersAck.end())
        {
            userOpts.insert(std::make_pair(user.getNickname(), &it->second));
            buf = ":" + _hostname + " 381 " + user.getNickname() + " :You are now an IRC operator";
        }
        else
            std::cerr << RED << "Something went wrong : OPER : No such user" << END << std::endl;
    }
    sendData(user.getSocket(), buf);
}

void IRCserver::_LIST(const Message &msg, const User &user)
{
    std::map<std::string, Channel>::const_iterator it;
    std::string buf;

    if (utils::toUpper(msg.getCommand()) != "LIST")
        return;
    buf = ":" + _hostname + " 321 " + user.getNickname() + " Channel :Users  Name";
    sendData(user.getSocket(), buf);
    if (msg.getParamets().size() == 0)
    {
        for (it = userChannels.begin(); it != userChannels.end(); ++it)
        {
            const Channel &channel = it->second;
            std::stringstream ss;

            ss << channel.getUsers().size();
            buf = ":" + _hostname + " 322 " + user.getNickname() + " " + channel.getName() + " " + ss.str() + " :" + channel.getTopic();
            sendData(user.getSocket(), buf);
        }
    }
    else
    {
        for (size_t i = 0; i < msg.getParamets().size(); ++i)
        {
            it = userChannels.find(msg.getParamets()[i]);

            if (it == userChannels.end())
                continue;

            const Channel &channel = it->second;
            std::stringstream ss;

            ss << channel.getUsers().size();
            buf = ":" + _hostname + " 322 " + user.getNickname() + " " + channel.getName() + " " + ss.str() + " :" + channel.getTopic();
            sendData(user.getSocket(), buf);
        }
    }
    buf = ":" + _hostname + " 323 " + user.getNickname() + " :End of /LIST";
    sendData(user.getSocket(), buf);
}

void IRCserver::_TOPIC(const Message &msg, const User &user)
{
    std::map<std::string, Channel>::iterator ch_it;

    std::string buf_string;
    std::string buf;

    if (utils::toUpper(msg.getCommand()) != "TOPIC")
        return;

    if (msg.getParamets().size() == 0)
    {
        buf = ":" + _hostname + " 461 " + user.getNickname() + " " + "TOPIC :Not enough parameters";
        sendData(user.getSocket(), buf);
        return;
    }

    buf_string = msg.getParamets()[0];
    if (buf_string[0] != '#' && buf_string[0] != '&')
    {

        buf = ":" + _hostname + " 403 " + user.getNickname() + " " + buf_string + " :No such channel";
        sendData(user.getSocket(), buf);
        return;
    }

    ch_it = userChannels.find(buf_string);

    if (ch_it != userChannels.end())
    {
        if (msg.getParamets().size() == 2)
        {
            ch_it->second.setTopic(msg.getParamets()[1]);

            buf = ":" + user.getNickname() + " TOPIC " + buf_string + " :" + ch_it->second.getTopic();

            sendData(user.getSocket(), buf);
            return;
        }
        else if (ch_it->second.getTopic().empty())
        {
            buf = ":" + _hostname + " 331 " + user.getNickname() + " " + buf_string + " :No topic is set";
            sendData(user.getSocket(), buf);
            return;
        }
        else if (!ch_it->second.getTopic().empty())
        {
            buf = ":" + _hostname + " 332 " + user.getNickname() + " " + buf_string + " :" + ch_it->second.getTopic();
            sendData(user.getSocket(), buf);
        }
    }
    else
    {
        buf = ":" + _hostname + " 403 " + user.getNickname() + " " + buf_string + " :No such channel";
        sendData(user.getSocket(), buf);
        return;
    }
}

void IRCserver::_NAMES(const Message &msg, const User &user)
{

    std::map<std::string, Channel>::iterator ch_it;
    std::string message;
    std::string buf;
    std::vector<std::string> buf_string;

    if (utils::toUpper(msg.getCommand()) != "NAMES")
        return;

    for (size_t i = 0; i < msg.getParamets().size(); ++i)
    {
        buf_string.push_back(msg.getParamets()[i]);
        if (!(!buf_string.empty() && (buf_string[i][0] == '#' || buf_string[i][0] == '&')))
            buf_string.pop_back();
    }

    ch_it = userChannels.begin();
    if (!buf_string.empty())
    {
        for (size_t i = 0; i < buf_string.size(); ++i)
        {
            std::map<std::string, User *>::const_iterator ch_us_it;
            std::vector<User>::const_iterator chusernameVec_it;
            ch_it = userChannels.find(buf_string[i]);
            if (ch_it != userChannels.end())
            {
                ch_us_it = ch_it->second.getUsers().begin();
                message = ":" + _hostname + " 353 " + user.getNickname() + " = " + ch_it->second.getName() + " :";
                for (; ch_us_it != ch_it->second.getUsers().end(); ++ch_us_it)
                {
                    chusernameVec_it = ch_it->second.getUsersByNickname(ch_us_it->second->getNickname());
                    if (chusernameVec_it != ch_it->second.getUsersByNicknames().end())
                        buf += "@" + ch_us_it->second->getNickname() + " ";
                    else
                        buf += ch_us_it->second->getNickname() + " ";
                }
                sendData(user.getSocket(), message + buf);
                message = ":" + _hostname + " 366 " + user.getNickname() + " " + ch_it->second.getName() + " :End of /NAMES list";
                sendData(user.getSocket(), message);
            }
            else
            {
                message = ":" + _hostname + " 366 " + user.getNickname() + " " + buf_string[i] + " :End of /NAMES list";
                sendData(user.getSocket(), message);
            }
            buf.clear();
        }
    }
}

void IRCserver::_INVITE(const Message &msg, const User &user)   {
    std::map<std::string, Channel>::iterator ch_it;
    std::map<std::string, User *>::const_iterator us_ch_it;
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);
    if (cmd != "INVITE")
        return;
    if (msg.getParamets().size() < 2)   {
        buf = ":" + _hostname + " 461 " + user.getNickname() + " " + "Not enough parameters sent for invite";
        sendData(user.getSocket(), buf);
        return;
    }
    buf = msg.getParamets()[1];
    if (buf[0] != '#' && buf[0] != '&')
        return;
    auto it  = usersAck.find(msg.getParamets()[0]);
    if (it == usersAck.end() || userChannels.empty()) {
        buf = ":" + _hostname + " 401 " + user.getNickname() + " " + msg.getParamets()[0] + " :No such nick or channel";
        sendData(user.getSocket(), buf);
        return;
    }
    auto channelsIt = userChannels.begin();
    channelsIt = ch_it->second.getUsers().find(user.getNickname());
    if (us_ch_it != ch_it->second.getUsers().end()) {
        us_ch_it = ch_it->second.getUsers().find(msg.getParamets()[0]);
        if (us_ch_it != ch_it->second.getUsers().end()) {
            buf = ":" + _hostname + " 443 " + user.getNickname() + " " + msg.getParamets()[0] + " :is already on channel" + buf_string;
            sendData(user.getSocket(), buf);
        }   else    {
            buf = ":" + _hostname + " 341 " + user.getNickname() + " " + msg.getParamets()[0] + " " + buf;
            sendData(user.getSocket(), buf);
            buf = ":" + user.getNickname() + " INVITE " + msg.getParamets()[0] + " :" + buf;
            sendData(it->second.getSocket(), buf);
        }
    }   else    {
        buf = ":" + _hostname + " 442 " + user.getNickname() + " #" + buf + " :You're not on that channel";
        sendData(user.getSocket(), buf);
        return;
    }
}

void IRCserver::quitCommad(const Message &msg, User **user)
{
    std::string buf;
    std::string cmd = msg.getCommand();
    std::transform(cmd.begin(), cmd.end(),cmd.begin(), ::toupper);
    if (cmd != "QUIT")
        return;
    if (msg.getParamets().empty())
        buf = ":" + (*user)->getNickname() + " Quit: " + (*user)->getNickname();
    else
        buf = ":" + (*user)->getNickname() + " Quit: " + msg.getParamets()[0];
    for (auto channelIt = userChannels.begin(); channelIt != userChannels.end(); ++channelIt)  {
        auto userIt = channelIt->second.getUsers().find((*user)->getNickname());
        if (userIt != channelIt->second.getUsers().end())
            sendDataToChannel(channelIt->second.getName(), buf, (*user)->getNickname());
    }
    buf = "Error, closing connection";
    sendData((*user)->getSocket(), buf);
    // removing the user
    close((*user)->getSocket());
    FD_CLR((*user)->getSocket(), &userFdSet);

    deleteUser((*user)->getNickname());

    *user = NULL;
}

void IRCserver::_KILL(const Message &msg, User **user)
{
    std::string buf;
    std::map<std::string, Channel>::const_iterator cit;
    std::map<std::string, User *>::const_iterator uit;

    if (utils::toUpper(msg.getCommand()) != "KILL")
        return;
    if (msg.getParamets().size() < 2)
    {
        buf = ":" + _hostname + " 461 KILL :Not enough parameters";
        sendData((*user)->getSocket(), buf);
        return;
    }
    if (userOpts.find((*user)->getNickname()) == userOpts.end())
    {
        buf = ":" + _hostname + " 481 " + (*user)->getNickname() + " :Permission Denied- You're not an IRC operator";
        sendData((*user)->getSocket(), buf);
        return;
    }
    if (usersAck.find(msg.getParamets()[0]) == usersAck.end())
    {
        buf = ":" + _hostname + " 401 " + msg.getParamets()[0] + " :No such nick";
        _send((*user)->getSocket(), buf);
        return;
    }
    buf = ":" + (*user)->getNickname() + " KILL " + msg.getParamets()[0] + " :" + msg.getParamets()[1];
    _send((*user)->getSocket(), buf);
    buf = ":localhost QUIT :Killed (" + (*user)->getNickname() + " (" + msg.getParamets()[1] + "))";

    // sending quit message to all
    User *killedUser = &usersAck.find(msg.getParamets()[0])->second;
    User tmp;
    tmp.setNickname(_hostname);
    quitCommad(Message(buf, tmp), &killedUser);
}

void IRCserver::_KICK(const Message &msg, const User &user)
{
    std::string buf;

    if (utils::toUpper(msg.getCommand()) != "KICK")
        return;
    if (msg.getParamets().size() < 2)
    {
        buf = ":" + _hostname + " 461 KILL :Not enough parameters";
        _send(user.getSocket(), buf);
        return;
    }
    if (userChannels.find(msg.getParamets()[0]) == userChannels.end())
    {
        buf = ":" + _hostname + " 403 " + user.getNickname() + " " + msg.getParamets()[0] + " :No such channel";
        _send(user.getSocket(), buf);
        return;
    }
    Channel &channel = userChannels.find(msg.getParamets()[0])->second;
    if (channel.getUsers().find(user.getNickname()) == channel.getUsers().end())
    {
        buf = ":" + _hostname + " 442 " + channel.getName() + " :You're not on that channel";
        _send(user.getSocket(), buf);
        return;
    }
    if (channel.getUsersByNickname(user.getNickname()) == channel.getUsersByNicknames().end())
    {
        buf = ":" + _hostname + " 482 " + user.getNickname() + " " + channel.getName() + " :You're not channel operator";
        _send(user.getSocket(), buf);
        return;
    }
    if (channel.getUsers().find(msg.getParamets()[1]) == channel.getUsers().end())
    {
        buf = ":" + _hostname + " 441 " + msg.getParamets()[1] + " " + channel.getName() + " :They aren't on that channel";
        _send(user.getSocket(), buf);
        return;
    }
    buf = ":" + user.getNickname() + " KICK " + msg.getParamets()[0] + " " + msg.getParamets()[1];
    _send(channel.getUsers().find(msg.getParamets()[1])->second->getSocket(), buf);
    // removing user
    channel.removeUser(msg.getParamets()[1]);
    // removing channel
    if (channel.getUsers().size() == 0)
    {
        userChannels.erase(channel.getName());
        return;
    }
    _sendToChannel(msg.getParamets()[0], buf, msg.getParamets()[1]);
}

void IRCserver::_execute(int socket, const std::string &buffer) {

    std::multimap<std::string, User>::iterator it;
    it = _users.begin();
    while (it != _users.end() && it->second.getSocket() != socket)
        it++;
    if (it == _users.end())
        return; // no such user

    Message msg(buf, it->second);
    User *user = &it->second;

    if (_password.empty() && !user->isPassworded())
        user->unablePassword();
    if (user->isPassworded() && user->isLogged())
    {
        quitCommad(msg, &user);
        if (user == NULL)
            return;
        _KILL(msg, &user);
        if (user == NULL)
            return;
    }

    _CAP(msg, *user);
    _PASS(msg, *user);
    _PING(msg, *user);
    if (user->isPassworded())
    {
        _NICK(msg, &user);
        _USER(msg, *user);
    }
    if (user->isPassworded() && user->isLogged())
    {
        privateMessageCommand(msg, *user);
        _NOTICE(msg);
        _JOIN(msg, *user);
        _PART(msg, *user);
        _LIST(msg, *user);
        _OPER(msg, *user);
        _KICK(msg, *user);
        _NAMES(msg, *user);
        _TOPIC(msg, *user);
        _INVITE(msg, *user);
    }
}
