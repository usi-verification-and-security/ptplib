cmake_minimum_required(VERSION 3.5)

execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/git_branch.sh OUTPUT_VARIABLE git_branch)

project(Protocol-Example)

find_package(Threads REQUIRED)

set(PTPLib_repository "https://github.com/usi-verification-and-security/partition-channel.git")

message("-- Protocol Example is fetching PTPLib from " ${PTPLib_repository})
message("-- Branch : ${git_branch}")

include(FetchContent)
FetchContent_Declare(
        PTPLib
        GIT_REPOSITORY ${PTPLib_repository}
        GIT_TAG ${git_branch}
)

FetchContent_MakeAvailable(PTPLib)

add_library(PTPLib::PTPLib ALIAS PTPLib)

message("-- PTPLib files have been written to: ${PTPLib_SOURCE_DIR}")

add_executable(Protocol-Example main.cc)

target_link_libraries(Protocol-Example PTPLib::PTPLib Threads::Threads)
