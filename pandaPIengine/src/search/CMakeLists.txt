file(GLOB search_SRC "*.h" "*.cpp")

add_library(search ${search_SRC})
target_compile_options(search  PUBLIC -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror=implicit-function-declaration "$<$<CONFIG:DEBUG>:-Og;-ggdb3>" "$<$<CONFIG:RELEASE>:-O3>")
target_link_options(search  PUBLIC "$<$<CONFIG:RELEASE>:-flto>")
