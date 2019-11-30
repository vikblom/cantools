cmake_minimum_required(VERSION 3.0)
project(cantomat)

add_executable(cantomat busassignment.c cantomat.c matwrite.c
  measurement.c messagedecoder.c messagehash.c signalformat.c)
target_compile_definitions(cantomat PRIVATE VERSION="0")
target_link_libraries(cantomat
  -l:libmatio.a -l:libhdf5.a -l:libszip.a -l:libz.a
  candbc canhash canblf)
