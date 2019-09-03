cmake_minimum_required(VERSION 3.0)
project(libcanvsb)

add_library(canvsb vsbreader.c)
target_include_directories(canasc INTERFACE .) # TODO: Limit public
