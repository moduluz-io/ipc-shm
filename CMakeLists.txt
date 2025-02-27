cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME ipc-shm)

project(${PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(src)

# Create the static library
file(GLOB_RECURSE SOURCES src/*.cpp src/*.cc)
file(GLOB_RECURSE HEADERS src/*.h src/*.hpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# Example executables
add_executable(server example/server.cc)
add_executable(client example/client.cc)
target_link_libraries(server PRIVATE ${PROJECT_NAME})
target_link_libraries(client PRIVATE ${PROJECT_NAME})