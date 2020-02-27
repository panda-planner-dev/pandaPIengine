file(GLOB intDataStructures_SRC "*.h" "*.cpp")

add_library(intDataStructures ${intDataStructures_SRC})
target_compile_options(intDataStructures PUBLIC ${compiler_flags})
target_link_options(intDataStructures PUBLIC ${linker_flags})

