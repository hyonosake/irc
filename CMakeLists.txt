cmake_minimum_required(VERSION 3.20)
project(irc)

set(CMAKE_CXX_STANDARD 14)

include_directories(server/includes)

add_executable(irc
        server/includes/IRCserverInterface.hpp
        server/srcs/IRCserverInterface.cpp
        server/includes/IRCserver.hpp
        server/srcs/IRCserver.cpp
        server/srcs/User.cpp
        server/includes/User.hpp
        server/includes/Message.hpp
        server/srcs/Message.cpp server/srcs/Channel.cpp server/includes/Channel.hpp)
