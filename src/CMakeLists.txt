cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)
set(CPLEX_SOURCE_DIR /home/gregor/Workspace/cplex)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SAT "Compile with the SAT planner" ON)


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
	list(APPEND linker_flags "$<$<CONFIG:RELEASE>:-flto>")
else (CPLEX_SOURCE_DIR)
	list(APPEND linker_flags "$<$<CONFIG:RELEASE>:-flto;-static;-static-libgcc>")
endif (CPLEX_SOURCE_DIR)



include_directories("search")
include_directories("heuristics")
include_directories("intDataStructures")
include_directories("sat")
add_subdirectory(search)
add_subdirectory(heuristics)
add_subdirectory(intDataStructures)
add_subdirectory(sat)


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
target_link_libraries(pandaPIengine PUBLIC heuristics)
target_link_libraries(pandaPIengine PUBLIC hrc)
target_link_libraries(pandaPIengine PUBLIC hlm)
target_link_libraries(pandaPIengine PUBLIC intDataStructures)

# if the user has specified a cplex installation he surely wants to use it
if (CPLEX_SOURCE_DIR)
	target_link_libraries(pandaPIengine PUBLIC ilpheuristics)
endif (CPLEX_SOURCE_DIR)

if (SAT)
	target_link_libraries(pandaPIengine PUBLIC sat)
endif (SAT)
