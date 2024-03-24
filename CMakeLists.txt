cmake_minimum_required(VERSION 3.10)
project(Server)

# Set source files for server
set(SERVER_SOURCES
    src/server/server.c
    src/cfg/support.c
    src/cfg/sock_config.c
    src/cfg/threads_config.c
)

# Set source files for client
set(CLIENT_SOURCES
    src/client/client.c
    src/cfg/support.c
    src/cfg/sock_config.c
    src/cfg/threads_config.c
)

# Set source files for test_client
set(TEST_CLIENT_SOURCES
    src/test_client/test_client.c
    src/cfg/support.c
)

# Include directories
include_directories(include)

# Add compiler flags for debug symbols
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")

# Create executable for server
add_executable(server ${SERVER_SOURCES})

# Set compiler flags for server target
set_target_properties(server PROPERTIES
    COMPILE_FLAGS "-Wall -DSERVER"
)

# Create executable for test_client
add_executable(test_client ${TEST_CLIENT_SOURCES})

# Set compiler flags for test_client target
set_target_properties(test_client PROPERTIES
    COMPILE_FLAGS "-Wall"
)

# Create executable for client
add_executable(client ${CLIENT_SOURCES})

# Set compiler flags for client target
set_target_properties(client PROPERTIES
    COMPILE_FLAGS "-Wall -DCLIENT"
)