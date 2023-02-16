cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)

# set(CPLEX_SOURCE_DIR /home/gregor/Workspace/cplex)
# set(CPLEX_SOURCE_DIR /home/dh/Programme/CPLEX_Studio1210)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SAT "Compile with the SAT planner" OFF)
option(BDD "Compile with the Symbolic planner" OFF)
##set(CPLEX_SOURCE_DIR /home/gregor/Workspace/cplex)


### if no build type is set, it is "Release"
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

project(pandaPIengine)

find_program(GENGETOPT gengetopt)

## required for IBM's CPLEX in order to force it to use newer names for the std headers
add_definitions(-DIL_STD)
list(APPEND compiler_flags -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic "$<$<CONFIG:DEBUG>:-Og;-ggdb3;-lpthread>" "$<$<CONFIG:RELEASE>:-O2;-lpthread>")

if (CPLEX_SOURCE_DIR)
	list(APPEND linker_flags "$<$<CONFIG:DEBUG>:-lpthread;-g>" "$<$<CONFIG:RELEASE>:-flto;-lpthread>")
else (CPLEX_SOURCE_DIR)
	#list(APPEND linker_flags "$<$<CONFIG:DEBUG>:-lpthread>" "$<$<CONFIG:RELEASE>:-flto;-lpthread;-static;-static-libgcc>")
	list(APPEND linker_flags "$<$<CONFIG:DEBUG>:-lpthread;-g>" "$<$<CONFIG:RELEASE>:-flto;-lpthread>")
	list(APPEND compiler_flags -DCMAKE_NO_ILP)
endif (CPLEX_SOURCE_DIR)

if (SAT)
else (SAT)
	list(APPEND compiler_flags -DCMAKE_NO_SAT)
endif(SAT)

if (BDD)
else (BDD)
	list(APPEND compiler_flags -DCMAKE_NO_BDD)
endif(BDD)



### Cudd Library Includes
include(ExternalProject)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/symbolic_search/cudd-3.0.0/cudd)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/symbolic_search/cudd-3.0.0/cplusplus)

### Project Includes
include_directories("search")
include_directories("symbolic_search")
include_directories("heuristics")
include_directories("intDataStructures")
include_directories("translation")
add_subdirectory(search)
add_subdirectory(symbolic_search)
add_subdirectory(heuristics)
add_subdirectory(intDataStructures)
add_subdirectory(translation)


add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/cmdline.h ${CMAKE_CURRENT_SOURCE_DIR}/cmdline.c
	COMMAND gengetopt --include-getopt --default-optional --unamed-opts -i options.ggo
	DEPENDS options.ggo
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "Generating getopt parser code (cmdline.{h,c}) ..."
	VERBATIM
)


file(GLOB main_SRC "*.h" "*.cpp" "cmdline.c")
 
# create the executable
add_executable (pandaPIengine ${main_SRC} "cmdline.c" "cmdline.h")
target_compile_options(pandaPIengine PUBLIC ${compiler_flags})
target_link_options(pandaPIengine PUBLIC ${linker_flags})

target_link_libraries(pandaPIengine PUBLIC search)
target_link_libraries(pandaPIengine PUBLIC symbolic_search)
target_link_libraries(pandaPIengine PUBLIC heuristics)
target_link_libraries(pandaPIengine PUBLIC hrc)
target_link_libraries(pandaPIengine PUBLIC hlm)
target_link_libraries(pandaPIengine PUBLIC translation)

if (BDD)
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
endif(BDD)

# if the user has specified a cplex installation he surely wants to use it
if (CPLEX_SOURCE_DIR)
	target_link_libraries(pandaPIengine PUBLIC ilpheuristics)
endif (CPLEX_SOURCE_DIR)

if (SAT)
	add_subdirectory(sat)
	include_directories("sat")
	target_link_libraries(pandaPIengine PUBLIC sat)
endif (SAT)

## must be added *after* SAT due to the use of the printMemory function, which is currently defined in the SAT module ... this is probably not a good idea.
target_link_libraries(pandaPIengine PUBLIC intDataStructures)
