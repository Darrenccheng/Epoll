cmake_minimum_required(VERSION 3.17)
project(EpollDemo)

set(CMAKE_CXX_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_executable(Server server.cpp)
add_executable(Client client.cpp)

target_link_libraries(Server pthread)