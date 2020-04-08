file(GLOB sat_SRC "*.h" "*.cpp")

add_library(sat ${sat_SRC})
target_compile_options(sat PUBLIC ${compiler_flags})
target_link_options(sat PUBLIC ${linker_flags})

target_link_libraries(sat ipasircryptominisat5)
