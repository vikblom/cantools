cmake_minimum_required(VERSION 3.0)
project(libcanblf)

add_library(canblf blfapi.c blfapi.h blfbuffer.c
  blfbuffer.h blfparser.c blfparser.h blfreader.c blfreader.h)
target_link_libraries(canblf PRIVATE candbc)
target_include_directories(canasc PUBLIC .) # TODO: Limit public
