cmake_minimum_required(VERSION 3.13)

option(SAT "Compile with the SAT planner" ON)

### if no build type is set, it is "Release"
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()


project(pandaPIengine)

list(APPEND compiler_flags -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic "$<$<CONFIG:DEBUG>:-Og;-ggdb3>" "$<$<CONFIG:RELEASE>:-O2>")
list(APPEND linker_flags "$<$<CONFIG:RELEASE>:-flto;-static;-static-libgcc>")


include_directories("search")
include_directories("heuristics")
include_directories("intDataStructures")
include_directories("sat")
add_subdirectory(search)
add_subdirectory(heuristics)
add_subdirectory(intDataStructures)
add_subdirectory(sat)

file(GLOB main_SRC "*.h" "*.cpp")
 
# create the executable
add_executable (pandaPIengine ${main_SRC})
target_compile_options(pandaPIengine PUBLIC ${compiler_flags})
target_link_options(pandaPIengine PUBLIC ${linker_flags})

target_link_libraries(pandaPIengine PUBLIC search)
target_link_libraries(pandaPIengine PUBLIC heuristics)
target_link_libraries(pandaPIengine PUBLIC hrc)
target_link_libraries(pandaPIengine PUBLIC hlm)
target_link_libraries(pandaPIengine PUBLIC intDataStructures)

if (SAT)
	target_link_libraries(pandaPIengine PUBLIC sat)
endif (SAT)
