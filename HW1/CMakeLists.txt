cmake_minimum_required(VERSION 3.21)
project(MTH9815_HW1)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Werror -pedantic -Wno-unused-result -Wno-deprecated-declarations")

add_executable(ex1 Ex1.cpp)
add_executable(ex2 Ex2.cpp DLinkedList.hpp)
add_executable(ex3 Ex3.cpp Hashtable.hpp)
