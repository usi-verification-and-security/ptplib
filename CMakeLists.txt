cmake_minimum_required(VERSION 3.12.3)
project(partition_channel)

set(CMAKE_CXX_STANDARD 17)

add_library(partition_channel PartitionChannelLibrary.h
        src/Channel.h src/Printer.h src/Timer.h src/PartitionConstant.h  src/Header.h src/lib.h src/Exception.h)

set_target_properties(partition_channel PROPERTIES LINKER_LANGUAGE CPP)