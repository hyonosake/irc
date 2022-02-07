#include "../includes/IRCserver.hpp"
#include "../includes/User.hpp"


static bool exitFlag = false;   // TODO: rename

void    sigintCatcher(int sig)  // TODO: rename
{
    if (sig == SIGINT)
        exitFlag = true;
}

IRCserver::IRCserver(uint32_t port, std::string password):
    IRCserverInterface(port, password)
{
    _delimeter = _DELIM;
    setHostname();
    setServerAdress();
    signal(SIGPIPE, SIG_IGN);
//    signal(SIGINT, sigintCatcher); // TODO: sigintCatcher
    initListener();
}

int32_t IRCserver::getMaxFd() const {
    return _max_fd;
}
void IRCserver::setMaxFd(int32_t maxFd) {
    _max_fd = maxFd;
}

int32_t IRCserver::getListener() const {
    return _listener;
}
void IRCserver::setListener(int32_t listener) {
    _listener = listener;
}

const fd_set &IRCserver::getClientFds() const {
    return _client_fds;
}
void IRCserver::setClientFds(const fd_set &clientFds) {
    _client_fds = clientFds;
}

const std::string &IRCserver::getBuffer() const {
    return _buffer;
}
void IRCserver::setBuffer(const std::string &buffer) {
    _buffer = buffer;
}

const std::map<std::string, User *> &IRCserver::getOperators() const {
    return _operators;
}
void IRCserver::setOperators(const std::map<std::string, User *> &operators) {
    _operators = operators;
}

const std::map<std::string, Channel> &IRCserver::getChannels() const {
    return _channels;
}

const std::string &IRCserver::getHostname() const {
    return _hostname;
}
void IRCserver::setHostname() {
    char hostname[_HOSTNAME_LEN];
    bzero((void*)(hostname), _HOSTNAME_LEN);
    if (gethostname(hostname, _HOSTNAME_LEN) != -1)
        _hostname = hostname;
}

sockaddr_in & IRCserver::getServerAdress() const {
    return _serverAdress;
}

void IRCserver::setServerAdress() {
    _serverAdress.sin_family = AF_INET;
    _serverAdress.sin_addr.s_addr = INADDR_ANY;
    _serverAdress.sin_port = htons(this->_port);
}

IRCserver::IRCserver() {}

void IRCserver::initListener() {

    FD_ZERO(&_client_fds);
    _listener = socket(AF_INET, SOCK_STREAM, getprotobyname("TCP")->p_proto);
    if (_listener < 0)
        throw std::invalid_argument(strerror(errno));
    _max_fd = _listener;

    int yes = 1;
    setsockopt(_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    int b = bind(_listener, (struct sockaddr *)&_serverAdress, sizeof(_serverAdress));
    if (b < 0)
        throw std::invalid_argument(strerror(errno));

    int l = listen(_listener, 10);
    if (l < 0)
        throw std::invalid_argument(strerror(errno));

    FD_SET(_listener, &_client_fds);
    _max_fd = _listener;
};

IRCserver::~IRCserver() = default;


void IRCserver::_accept()
{
    struct sockaddr_in temp;

    socklen_t socklen = sizeof(struct sockaddr_in);
    int new_fd = accept(_listener, (struct sockaddr *)&temp, &socklen);
    if (new_fd == -1)
        throw std::invalid_argument(strerror(errno));
    fcntl(_listener, F_SETFL, O_NONBLOCK);
    _addUser(new_fd);
    if (_max_fd < new_fd)
        _max_fd = new_fd;
    FD_SET(new_fd, &_client_fds);
    std::cout << CYAN << "New connetion on socket : " << new_fd << RESET << '\n';   // TODO: change message
}

void IRCserver::_addUser(int socket) {
        User created;
        created.setSocket(socket);
        std::pair<std::string, User> tmp(std::string(""), created);
        _users.insert(tmp);
}

void IRCserver::_addUser(const User &user)
{
    std::pair<std::string, User> tmp(user.getNickname(), user);
    _users.insert(tmp);
}

void IRCserver::start()
{
    std::unordered_multimap<std::string, User>::iterator uit;

    fd_set select_fds;
    select_fds = _client_fds;

    std::cout << GREEN << "Server is started..." << RESET << std::endl; // TODO: change message
    while (select(_max_fd + 1, &select_fds, nullptr, nullptr, nullptr) != -1)
    {
        if (exitFlag)
            _stop();
        for (int i = 3; i < _max_fd + 1; i++)
        {
            if (!FD_ISSET(i, &select_fds) || i == _listener)
                continue;
            std::string buf;
            uit = _getUser(i);
            User *user = &(uit->second);

            if (!user->getSendBuffer().empty())
                _send(i, user->getSendBuffer());

            try
            {
                _recv(i, buf);
            }
            catch (const std::exception &e)
            {
                _QUIT(Message(std::string("QUIT :Remote host closed the connection"), *user), &user);
            }
            _execute(i, buf);

            std::cout << "Number of users :\t "
                      << _users.size() << std::endl;
            int logged = 0;
            for (uit = _users.begin(); uit != _users.end(); ++uit)
                if (uit->second.isLogged())
                    logged++;
            std::cout << "Number of logged users : "
                      << logged << std::endl;
        }
        if (FD_ISSET(_listener, &select_fds))
            _accept();
        select_fds = _client_fds;
    }
    FD_ZERO(&select_fds);
    FD_ZERO(&_client_fds);
    throw std::invalid_argument(strerror(errno));
}



void    IRCserver::_stop()
{
    std::unordered_multimap<std::string, User>::iterator    it;

    for (it = _users.begin(); it != _users.end(); ++it)
    {
        User *user = &it->second;
        _QUIT(Message("", it->second), &user);
    }
    FD_ZERO(&_client_fds);
}


