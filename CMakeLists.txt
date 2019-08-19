cmake_minimum_required(VERSION 3.0)
project(cantools)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed.")
endif()

add_subdirectory(src/libcandbc)
add_subdirectory(src/dbccopy)
add_subdirectory(src/dbcls)
add_subdirectory(src/matdump)
