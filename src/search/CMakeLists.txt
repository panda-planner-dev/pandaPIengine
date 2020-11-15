file(GLOB search_SRC "*.h" "*.cpp")

add_library(search ${search_SRC} primeNumbers.h)
target_compile_options(search PUBLIC ${compiler_flags})
target_link_options(search PUBLIC ${linker_flags})
