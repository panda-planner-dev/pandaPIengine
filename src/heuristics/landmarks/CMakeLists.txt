file(GLOB hlm_SRC "*.h" "*.cpp" "./lmDataStructures/*.cpp" "./lmDataStructures/*.h" "./lmExtraction/*.cpp" "./lmExtraction/*.h")

add_library(hlm ${hlm_SRC})
target_compile_options(hlm PUBLIC ${compiler_flags})
target_link_options(hlm PUBLIC ${linker_flags})
target_link_libraries(hlm PUBLIC hrc)
target_link_libraries(hlm PUBLIC heuristics)
