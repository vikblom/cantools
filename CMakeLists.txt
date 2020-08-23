cmake_minimum_required(VERSION 3.0)
project(cantools)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed.")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
#if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
#endif()

add_subdirectory(src/cantools)

add_subdirectory(src/libcandbc)
#add_subdirectory(src/libcanasc)
add_subdirectory(src/libcanblf)
#add_subdirectory(src/libcanclg)
#add_subdirectory(src/libcanvsb)
add_subdirectory(src/hashtable)

add_subdirectory(apps/dbccopy)
add_subdirectory(apps/dbcls)
add_subdirectory(apps/matdump)
