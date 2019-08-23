cmake_minimum_required(VERSION 3.0)
project(libcandbc)

# Use checked in lexer and parser instead of regenerating every build.
#find_package(BISON REQUIRED)
#find_package(FLEX REQUIRED)
#BISON_TARGET(DbcParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
#FLEX_TARGET(DbcLexer lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
#ADD_FLEX_BISON_DEPENDENCY(DbcLexer DbcParser)
#add_library(candbc dbcmodel.c dbcreader.c dbcwriter.c
#  ${BISON_DbcParser_OUTPUTS} ${FLEX_DbcLexer_OUTPUTS})

add_library(candbc dbcmodel.c dbcreader.c dbcwriter.c lexer.c parser.c)
target_link_libraries(candbc -lm)
target_include_directories(candbc PRIVATE . PUBLIC .) # TODO: Limit public
