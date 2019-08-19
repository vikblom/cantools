cmake_minimum_required(VERSION 3.0)
project(matdump)

add_executable(matdump matdump.c)
target_link_libraries(matdump -lmatio)
target_compile_definitions(matdump PRIVATE PACKAGE_VERSION="0")
