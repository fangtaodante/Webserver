cmake_minimum_required(VERSION 3.13)
project(NewServer C)

set(CMAKE_C_STANDARD 99)

add_executable(NewServer main.c server_epoll.c server_epoll.h wrap.c wrap.h)
