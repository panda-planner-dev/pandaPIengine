file(GLOB translation_SRC "*.h" "*.cpp")

add_library(translation ${translation_SRC})
target_compile_options(translation PUBLIC ${compiler_flags})
target_link_options(translation PUBLIC ${linker_flags})
