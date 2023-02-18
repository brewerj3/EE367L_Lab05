cmake_minimum_required(VERSION 3.25)
project(EE367L_Lab05 C)

set(CMAKE_C_STANDARD 99)

add_executable(net367
        main.c main.h
        man.c man.h
        packet.c packet.h
        host.c host.h
        net.c net.h)
