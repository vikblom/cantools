cmake_minimum_required(VERSION 3.0)
project(libcanclg)

add_library(canclg clgreader.c)
target_include_directories(canasc PUBLIC .) # TODO: Limit public
