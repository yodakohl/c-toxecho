
cmake_minimum_required (VERSION 2.8.11)
project (echoBot)

add_definitions( -std=c++11)
add_executable (echoBot echo.cpp av.cpp)
target_link_libraries (echoBot pthread toxcore toxav sodium)
