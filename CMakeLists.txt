cmake_minimum_required(VERSION 3.16)
project(projects C)

set(CMAKE_C_STANDARD 11)

add_executable(server
        server/server.c
        common/world.c
        common/world.h
        common/ipc.c
        common/ipc.h
        common/randomWalk.c
        common/randomWalk.h
        common/protocol.h
        common/printer.c
        common/printer.h
)

add_executable(client
        client/client.c
        common/world.c
        common/world.h
        common/ipc.c
        common/ipc.h
        common/randomWalk.h
        common/protocol.h
        common/printer.c
        common/printer.h
)

# Link math library
target_link_libraries(client PRIVATE m)
target_link_libraries(server PRIVATE m)
