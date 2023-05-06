cmake_minimum_required(VERSION 3.25)
project(EE367L_Lab05 C)

set(CMAKE_C_STANDARD 99)

add_executable(net367
        main.c main.h
        man.c man.h
        packet.c packet.h
        host.c host.h
        net.c net.h
        switch2.c switch2.h
        server.c server.h)

file(COPY p2p.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY p2ps1.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY p2ps2.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY psp.config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY LabSwitch DESTINATION ${CMAKE_BINARY_DIR})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")