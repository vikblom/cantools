cmake_minimum_required(VERSION 3.0)
project(dbccopy)

add_executable(dbccopy dbccopy.c)
target_link_libraries(dbccopy candbc)
