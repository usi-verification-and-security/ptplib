cmake_minimum_required(VERSION 3.5)

project(PTP-Example)

find_package(PTPLib CONFIG REQUIRED)

add_executable(PTP-Example src/main.cc)
target_link_libraries(PTP-Example PTPLib::PTPLib)
