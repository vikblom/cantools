cmake_minimum_required(VERSION 3.0)
project(libcanasc)

add_library(canasc ascreader.c)
target_include_directories(canasc INTERFACE .) # TODO: Limit public
