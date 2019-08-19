cmake_minimum_required(VERSION 3.0)
project(libcanasc)

add_library(canasc ascreader.c)
target_link_libraries(canasc candbc cantomat)
#target_include_directories(canasc PRIVATE . PUBLIC .) # TODO: Limit public
