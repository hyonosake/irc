#include <utility>

#include "../includes/Channel.hpp"
#include "../includes/IRCserver.hpp"

Channel::Channel() {}

Channel::~Channel() {}

Channel::Channel(std::string name) :channelName(std::move(name)), userLimit(10) {}

void Channel::addUser(User &new_user)   {
    if(usersAck.size() > userLimit)  {
        std::cout << BLUE << "Unable to insert new user: not enough sockets availible\n" << RESET;
        return;
    }
    usersAck.insert(std::make_pair(new_user.getNickname(), &new_user));
}

void Channel::addUsernameVec(User &newUsernameVec) {
    usernameVec.push_back(newUsernameVec);
}

const std::map<std::string, User *> &Channel::getUsers() const  {
    return usersAck;
}

const std::vector<User> &Channel::getUsersByNicknames() const   {
    return usernameVec;
}

const std::string &Channel::getName() const {
    return channelName;
}

const std::string &Channel::getTopic() const    {
    return channelTopic;
}

bool Channel::setName(std::string name) {
    channelName = std::move(name);
    return(true);
}

void Channel::setTopic(std::string topic)   {
    channelTopic = std::move(topic);
}

bool Channel::removeUser(const std::string& removeChannelName)  {
    if(usersAck.erase(removeChannelName))
        return(true);
    return(false);
}

bool Channel::RemoveNicknameUsers(std::string const& nick) {
    for(auto it = usernameVec.begin(); it != usernameVec.end(); it++)    {
        if(it->getNickname() == nick)   {
            usernameVec.erase(it);
            return true;
        }
    }
    return false;
}

std::vector<User>::const_iterator Channel::getUsersByNickname(std::string const &nick) const  {
    for (auto it = usernameVec.begin(); it != usernameVec.end(); it++) {
        if(it->getNickname() == nick)   {
            return it;
        }
    }
    return usernameVec.end();
}

void Channel::changeTopic(std::string newChannelTopic)    {
    channelTopic = std::move(newChannelTopic);
}

const std::string &Channel::getPass() const {
    return channelPassword;
}

void Channel::setPass(std::string pass) {
    channelPassword = std::move(pass);
}
