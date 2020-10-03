cmake_minimum_required(VERSION 3.0)

add_library(cantools
  busassignment.c matwrite.c measurement.c
  messagedecoder.c messagehash.c signalformat.c)

target_include_directories(cantools PUBLIC .)

IF(WIN32)
  target_link_libraries(cantools PRIVATE
    -l:libmatio.a -l:libhdf5.a -l:libszip.a -l:libz.a
    candbc canhash canblf)
ELSE()
  target_link_libraries(cantools PRIVATE
    -lmatio -lz candbc canhash canblf)
ENDIF()
