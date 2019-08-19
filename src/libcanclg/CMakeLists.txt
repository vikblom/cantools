cmake_minimum_required(VERSION 3.0)
project(libcanclg)

add_library(canclg clgreader.c)
target_link_libraries(canclg candbc cantomat)
#target_include_directories(canasc PRIVATE . PUBLIC .) # TODO: Limit public
