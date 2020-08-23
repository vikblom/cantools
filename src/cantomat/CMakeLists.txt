cmake_minimum_required(VERSION 3.0)
project(cantomat)

add_library(cantomat busassignment.c cantomat.c matwrite.c
  measurement.c messagedecoder.c messagehash.c signalformat.c)

target_compile_definitions(cantomat PRIVATE VERSION="0")
target_include_directories(cantomat PUBLIC .)

IF(WIN32)
  target_link_libraries(cantomat PRIVATE
    -l:libmatio.a -l:libhdf5.a -l:libszip.a -l:libz.a
    candbc canhash canblf)
ELSE()
  target_link_libraries(cantomat PRIVATE
    -lmatio -lz candbc canhash canblf)
ENDIF()
