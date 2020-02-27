add_subdirectory(landmarks)
add_subdirectory(rcHeuristics)

file(GLOB heuristics_SRC "*.h" "*.cpp")

add_library(heuristics ${heuristics_SRC})
target_compile_options(heuristics  PUBLIC -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic "$<$<CONFIG:DEBUG>:-Og;-ggdb3>" "$<$<CONFIG:RELEASE>:-O3>")
target_link_options(heuristics PUBLIC "$<$<CONFIG:RELEASE>:-flto>")
