cmake_minimum_required(VERSION 4.1)
project(semestralka C)

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
        common/ipc.h)

# Link Winsock2 on Windows/MinGW
if (WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
endif()
