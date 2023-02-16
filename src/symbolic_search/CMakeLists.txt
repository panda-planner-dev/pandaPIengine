file(GLOB symbolic_search_SRC "*.h" "*.cpp")

add_library(symbolic_search ${symbolic_search_SRC})
target_compile_options(symbolic_search PUBLIC ${compiler_flags})
target_link_options(symbolic_search PUBLIC ${linker_flags})
