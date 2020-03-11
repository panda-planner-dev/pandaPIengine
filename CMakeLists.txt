cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

### if no build type is set, it is "Release"
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()


project(pandaPIengine)

## required for IBM's CPLEX in order to force it to use newer names for the std headers
add_definitions(-DIL_STD)
list(APPEND compiler_flags -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic "$<$<CONFIG:DEBUG>:-Og;-ggdb3>" "$<$<CONFIG:RELEASE>:-O2>")
if (CPLEX_SOURCE_DIR)
	list(APPEND linker_flags "$<$<CONFIG:RELEASE>:-flto>")
else (CPLEX_SOURCE_DIR)
	list(APPEND linker_flags "$<$<CONFIG:RELEASE>:-flto;-static;-static-libgcc>")
endif (CPLEX_SOURCE_DIR)


include_directories("src")
add_subdirectory(src)
