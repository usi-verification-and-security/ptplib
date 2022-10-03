cmake_minimum_required(VERSION 3.5)

project(Protocol-Example)

find_package(Threads REQUIRED)

find_package(PTPLib CONFIG REQUIRED)

add_executable(Protocol-Example main.cc)
target_link_libraries(Protocol-Example PTPLib::PTPLib Threads::Threads)