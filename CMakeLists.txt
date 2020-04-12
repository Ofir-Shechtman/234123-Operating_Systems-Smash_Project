cmake_minimum_required(VERSION 3.13)
project(OS_Wet1)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

#add_custom_target(OS_Wet1 COMMAND make
#        CLION_EXE_DIR=${PROJECT_BINARY_DIR})


add_executable(OS_Wet1
        smash.cpp
        Commands.cpp
        signals.cpp)