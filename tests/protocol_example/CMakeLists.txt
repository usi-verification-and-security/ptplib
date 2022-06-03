cmake_minimum_required(VERSION 3.5)

set(PTPLib_branch ${branch})
if(NOT PTPLib_branch )
    set(PTPLib_branch master)
endif()

project(Protocol-Example)

find_package(Threads REQUIRED)

set(PTPLib_repository "https://github.com/usi-verification-and-security/ptplib.git")

message(STATUS "Protocol Example is fetching PTPLib from " ${PTPLib_repository})
message(STATUS "PTPLib Branch : ${PTPLib_branch}")

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
        PTPLib
        GIT_REPOSITORY ${PTPLib_repository}
        GIT_TAG ${PTPLib_branch}
        GIT_SHALLOW true
        GIT_PROGRESS true
)

FetchContent_MakeAvailable(PTPLib)

add_library(PTPLib::PTPLib ALIAS PTPLib)

message(STATUS "PTPLib include: ${PTPLib_SOURCE_DIR}/include")

add_executable(Protocol-Example main.cc)

target_link_libraries(Protocol-Example PTPLib::PTPLib Threads::Threads)
