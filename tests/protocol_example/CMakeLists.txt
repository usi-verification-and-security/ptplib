cmake_minimum_required(VERSION 3.5)

set(PTPLib_branch ${branch})
if(NOT PTPLib_branch )
    set(PTPLib_branch protocol_example)
endif()

project(Protocol-Example)

find_package(Threads REQUIRED)

set(PTPLib_repository "https://github.com/usi-verification-and-security/partition-channel.git")

message("-- Protocol Example is fetching PTPLib from " ${PTPLib_repository})
message("-- PTPLib Branch : ${PTPLib_branch}")

include(FetchContent)
FetchContent_Declare(
        PTPLib
        GIT_REPOSITORY ${PTPLib_repository}
        GIT_TAG ${PTPLib_branch}
)

FetchContent_MakeAvailable(PTPLib)

add_library(PTPLib::PTPLib ALIAS PTPLib)

message("-- PTPLib files have been written to: ${PTPLib_SOURCE_DIR}")

add_executable(Protocol-Example main.cc)

target_link_libraries(Protocol-Example PTPLib::PTPLib Threads::Threads)
