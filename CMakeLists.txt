cmake_minimum_required(VERSION 3.0)
project(cantools)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed.")
endif()

# FIXME: Ugly hack
include_directories(src/hashtable src/cantomat)

add_subdirectory(src/cantomat)

add_subdirectory(src/libcandbc)
add_subdirectory(src/libcanasc)
add_subdirectory(src/libcanblf)
add_subdirectory(src/libcanclg)
add_subdirectory(src/libcanvsb)
add_subdirectory(src/hashtable)

add_subdirectory(src/dbccopy)
add_subdirectory(src/dbcls)
add_subdirectory(src/matdump)
