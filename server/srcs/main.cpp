#include "../includes/IRCserver.hpp"

int main(int ac, char *av[])    {
    if (ac != 3)  {
        std::cout << RED << "valid params need" << RESET << '\n';
        return 1;
    }
    int port = static_cast<int>(std::atoi(av[1]));
    std::string password(av[2]);
    try {
        IRCserver server(port, password);
        server.start();
    }   catch (const std::exception &e) {
        std::cerr << RED << e.what() << RESET << '\n';
    }
    return 0;
}

