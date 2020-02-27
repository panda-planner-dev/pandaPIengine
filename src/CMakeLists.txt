cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

### if no build type is set, it is "Release"
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()


project(pandaPIPlanner)

include_directories("search")
include_directories("heuristics")
include_directories("intDataStructures")
add_subdirectory(search)
add_subdirectory(heuristics)
add_subdirectory(intDataStructures)

file(GLOB main_SRC "*.h" "*.cpp")
 
# create the executable
add_executable (pandaPIplanner ${main_SRC})
target_compile_options(pandaPIplanner PUBLIC -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic "$<$<CONFIG:DEBUG>:-Og;-ggdb3>"
"$<$<CONFIG:RELEASE>:-O2>")
target_link_options(pandaPIplanner PUBLIC "$<$<CONFIG:RELEASE>:-flto;-static;-static-libgcc>")
#target_compile_features(pandaPIplanner PUBLIC cxx_std_17)

target_link_libraries(pandaPIplanner PUBLIC search)
target_link_libraries(pandaPIplanner PUBLIC heuristics)
target_link_libraries(pandaPIplanner PUBLIC hrc)
target_link_libraries(pandaPIplanner PUBLIC hlm)
target_link_libraries(pandaPIplanner PUBLIC intDataStructures)
