cmake_minimum_required(VERSION 3.0)

add_library(cantools
  busassignment.c matwrite.c h5write.c measurement.c
  messagedecoder.c messagehash.c signalformat.c
  writer.c)

target_include_directories(cantools PUBLIC .)

# DEP: HDF5
find_package(HDF5 COMPONENTS C HL)
if(NOT HDF5_FOUND)
  message(FATAL_ERROR "HDF5 could not be found!")
endif(NOT HDF5_FOUND)
target_include_directories(cantools PRIVATE ${HDF5_INCLUDE_DIRS})
target_link_libraries(cantools PRIVATE ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})


# DEP: MATIO
IF(WIN32)
  target_link_libraries(cantools PUBLIC
    -l:libmatio.a -l:libhdf5.a -l:libszip.a -l:libz.a
    candbc canhash canblf)
ELSE()
  target_link_libraries(cantools PUBLIC
    -lmatio -lz candbc canhash canblf)
ENDIF()
