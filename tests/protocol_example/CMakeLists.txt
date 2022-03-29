cmake_minimum_required(VERSION 3.5)

project(Protocol-Example)

find_package(Threads REQUIRED)

set(PTPLibRepo "https://github.com/usi-verification-and-security/partition-channel.git")

message("-- Protocol Example is fetching PTPLib from " ${PTPLibRepo})

include(FetchContent)
FetchContent_Declare(
        PTPLib
        GIT_REPOSITORY ${PTPLibRepo}
        GIT_TAG protocol_example
)

FetchContent_MakeAvailable(PTPLib)

add_library(PTPLib::PTPLib ALIAS PTPLib)

message("-- PTPLib files have been written to: ${PTPLib_SOURCE_DIR}")

add_executable(Protocol-Example main.cc)

target_link_libraries(Protocol-Example PTPLib::PTPLib Threads::Threads)
