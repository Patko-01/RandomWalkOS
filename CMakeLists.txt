cmake_minimum_required(VERSION 3.16)
project(projects C)

set(CMAKE_C_STANDARD 11)

add_executable(server
        server/server.c
        server/server.h
        common/world.c
        common/world.h
        common/ipc.c
        common/ipc.h)

add_executable(client
        client/client.c
        client/client.h
        common/world.c
        common/world.h
        common/ipc.c
        common/ipc.h
)
