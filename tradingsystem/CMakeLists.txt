cmake_minimum_required(VERSION 3.21)
project(MTH9815_Project)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Werror -pedantic -Wno-unused-result -Wno-deprecated-declarations")
include_directories(/usr/local/boost_1_78_0/)
link_directories(/usr/local/boost_1_78_0/libs/)

add_executable(main main.cpp marketdataservice.hpp pricingservice.hpp tradebookingservice.hpp positionservice.hpp soa.hpp products.hpp riskservice.hpp executionservice.hpp)