cmake_minimum_required(VERSION 3.0)

add_library(canblf
  blfapi.c blfapi.h
  blfbuffer.c blfbuffer.h
  blfreader.c blfreader.h)
target_link_libraries(canblf PRIVATE cantools candbc -lz)
target_include_directories(canblf PUBLIC .) # TODO: Limit public
set_property(TARGET canblf PROPERTY C_STANDARD 90)
