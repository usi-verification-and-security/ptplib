cmake_minimum_required(VERSION 3.5)

project(Thread-Example)
set(partitionChannelRepo "https://github.com/usi-verification-and-security/partition-channel.git")
message("opensmt is fetching partition-channel library from " ${partitionChannelRepo})
include(FetchContent)

FetchContent_Declare(
        PTPLib
        GIT_REPOSITORY ${partitionChannelRepo}
        GIT_TAG master
)

FetchContent_MakeAvailable(PTPLib)

add_library(PTPLib::PTPLib ALIAS PTPLib)

message("-- partition-channel files have been written to: ${PTPLib_SOURCE_DIR}")


add_executable(Thread-Example main.cc)

target_link_libraries(Thread-Example PTPLib::PTPLib)
