add_subdirectory(landmarks)
add_subdirectory(rcHeuristics)

file(GLOB heuristics_SRC "*.h" "*.cpp")

add_library(heuristics ${heuristics_SRC})
target_compile_options(heuristics PUBLIC ${compiler_flags})
target_link_options(heuristics PUBLIC ${linker_flags})

