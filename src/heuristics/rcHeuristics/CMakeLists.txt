file(GLOB hrc_SRC "*.h" "*.cpp")

add_library(hrc ${hrc_SRC})
target_compile_options(hrc PUBLIC ${compiler_flags})
target_link_options(hrc PUBLIC ${linker_flags})
