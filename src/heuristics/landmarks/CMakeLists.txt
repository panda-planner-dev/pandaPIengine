file(GLOB hlm_SRC "*.h" "*.cpp" "./lmDataStructures/*.cpp" "./lmDataStructures/*.h" "./lmExtraction/*.cpp" "./lmExtraction/*.h")

add_library(hlm ${hlm_SRC} lmExtraction/LmAoNode.cpp lmExtraction/LmAoNode.h lmExtraction/LmCausal.cpp lmExtraction/LmCausal.h)
target_compile_options(hlm PUBLIC ${compiler_flags})
target_link_options(hlm PUBLIC ${linker_flags})
