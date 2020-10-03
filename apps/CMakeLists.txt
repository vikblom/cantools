cmake_minimum_required(VERSION 3.0)

exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe --tags"
    OUTPUT_VARIABLE GIT_VERSION)

# CanToMat
add_executable(cantomat cantomat.c)
target_link_libraries(cantomat cantools candbc canhash canblf)
target_compile_definitions(cantomat PRIVATE VERSION="${GIT_VERSION}")

# DbcCopy
add_executable(dbccopy dbccopy.c)
target_link_libraries(dbccopy candbc)

# DbcLs
add_executable(dbcls dbcls.c)
target_link_libraries(dbcls candbc)

# MatDump
add_executable(matdump matdump.c)
target_link_libraries(matdump -lmatio)
