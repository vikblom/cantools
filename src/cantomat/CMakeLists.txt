cmake_minimum_required(VERSION 3.0)
project(cantomat)

add_executable(cantomat busassignment.c cantomat.c matwrite.c
  measurement.c messagedecoder.c messagehash.c signalformat.c)
target_compile_definitions(cantomat PRIVATE VERSION="0")
target_link_libraries(cantomat
  -lmatio -lz
  candbc canhash canasc canblf canclg canvsb)
