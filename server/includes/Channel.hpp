#ifndef IRC_CHANNEL_HPP
#define IRC_CHANNEL_HPP

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <utility>
#include <vector>
#include <map>

#include "User.hpp"

class Channel {
private:
    typedef std::map<std::string, User*> userMap;
    std::string				channelName;
    std::string				channelTopic;
    std::string				channelPassword;
    userMap		            usersAck;
    std::vector<User>		usernameVec;
    size_t					userLimit;

private:
    Channel();

public:
    Channel(std::string name);
    ~Channel();

    void addUser(User &rhs);
    void addUsernameVec(User &_user);
    bool RemoveNicknameUsers(const std::string& nick);
    bool removeUser(const std::string& userNickname);
    void changeTopic(std::string newTopic);
    const std::string			&getName() const;
    const std::string			&getTopic() const;
    const userMap       		&getUsers() const;
    const std::vector<User>		&getUsersByNicknames() const;
    const std::string			&getPass() const;
    std::vector<User>::const_iterator getUsersByNickname(std::string const &nick) const;

    bool				  setName(std::string name);
    void				  setTopic(std::string topic);
    void				  setPass (std::string pass);
};

#endif

#endif //IRC_CHANNEL_HPP
