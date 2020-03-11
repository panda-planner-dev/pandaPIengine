include_directories("search")
include_directories("heuristics")
include_directories("intDataStructures")
add_subdirectory(search)
add_subdirectory(heuristics)
add_subdirectory(intDataStructures)

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
# if the user has specified a cplex installation he surely wants to use it
if (CPLEX_SOURCE_DIR)
	target_link_libraries(pandaPIplanner PUBLIC ilpheuristics)
endif (CPLEX_SOURCE_DIR)

