cmake_minimum_required(VERSION 3.20)
project(irc VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

include_directories(server/includes)


add_compile_options(
        -Werror

        -Wall
        -Wextra
        -Wpedantic

        -Wcast-align
        -Wcast-qual
        -Wconversion
        -Wctor-dtor-privacy
        -Wenum-compare
        -Wfloat-equal
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wredundant-decls
        -Wsign-conversion
        -Wsign-promo
)

add_executable(irc
        server/includes/IRCserverInterface.hpp
        server/srcs/IRCserverInterface.cpp
        server/includes/IRCserver.hpp
        server/srcs/IRCserver.cpp
        server/srcs/User.cpp
        server/includes/User.hpp
        server/includes/Message.hpp
        server/srcs/Message.cpp
        server/srcs/Channel.cpp server/includes/Channel.hpp
        server/srcs/main.cpp)
