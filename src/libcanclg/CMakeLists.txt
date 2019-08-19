cmake_minimum_required(VERSION 3.0)
project(libcanclg)

add_library(canclg clgreader.c)
target_link_libraries(canclg PRIVATE candbc)
target_include_directories(canasc PUBLIC .) # TODO: Limit public
