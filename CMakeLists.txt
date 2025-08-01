cmake_minimum_required(VERSION 3.20)

project(LogLiteApp CXX)

# Use C++20 for jthread, format and source_location
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(LogLiteApp src/main.cpp)

target_include_directories(LogLiteApp PUBLIC include)