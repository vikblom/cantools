cmake_minimum_required(VERSION 3.0)
project(dbcls)

add_executable(dbcls dbcls.c)
target_link_libraries(dbcls candbc)
