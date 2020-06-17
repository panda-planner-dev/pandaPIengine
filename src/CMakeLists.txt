cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

### if no build type is set, it is "Release"
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()


project(pandaPIengine)

list(APPEND compiler_flags -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic "$<$<CONFIG:DEBUG>:-Og;-ggdb3>" "$<$<CONFIG:RELEASE>:-O2>")
list(APPEND linker_flags "$<$<CONFIG:RELEASE>:-flto;-static;-static-libgcc>")

### Cudd Library Includes
include(ExternalProject)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/symbolic_search/cudd-3.0.0/cudd)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/symbolic_search/cudd-3.0.0/cplusplus)

### Project Includes
include_directories("search")
include_directories("symbolic_search")
include_directories("heuristics")
include_directories("intDataStructures")
add_subdirectory(search)
add_subdirectory(symbolic_search)
add_subdirectory(heuristics)
add_subdirectory(intDataStructures)

file(GLOB main_SRC "*.h" "*.cpp")
 
# create the executable
add_executable (pandaPIengine ${main_SRC})
target_compile_options(pandaPIengine PUBLIC ${compiler_flags})
target_link_options(pandaPIengine PUBLIC ${linker_flags})

target_link_libraries(pandaPIengine PUBLIC search)
target_link_libraries(pandaPIengine PUBLIC symbolic_search)
target_link_libraries(pandaPIengine PUBLIC heuristics)
target_link_libraries(pandaPIengine PUBLIC hrc)
target_link_libraries(pandaPIengine PUBLIC hlm)
target_link_libraries(pandaPIengine PUBLIC intDataStructures)

### Build Cudd
ExternalProject_add(
        libcudd
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/symbolic_search/cudd-3.0.0/
        CONFIGURE_COMMAND autoreconf ${CMAKE_SOURCE_DIR}/symbolic_search/cudd-3.0.0/ &&  ${CMAKE_SOURCE_DIR}/symbolic_search/cudd-3.0.0/configure --enable-obj --enable-silent-rules "CFLAGS=-O3" "CXXFLAGS=-std=c++0x -O3"
        BUILD_COMMAND make
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE 0
)

### Link Cudd
add_dependencies(pandaPIengine libcudd)
target_link_libraries(pandaPIengine PUBLIC ${CMAKE_BINARY_DIR}/libcudd-prefix/src/libcudd-build/cudd/.libs/libcudd.a)
