add_subdirectory(landmarks)
add_subdirectory(rcHeuristics)

if (CPLEX_SOURCE_DIR)
	add_subdirectory(dofHeuristics)
endif (CPLEX_SOURCE_DIR)

file(GLOB heuristics_SRC "*.h" "*.cpp")

add_library(heuristics ${heuristics_SRC})
target_compile_options(heuristics PUBLIC ${compiler_flags})
target_link_options(heuristics PUBLIC ${linker_flags})
