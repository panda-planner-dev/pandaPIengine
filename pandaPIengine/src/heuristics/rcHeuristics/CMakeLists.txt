file(GLOB hrc_SRC "*.h" "*.cpp")

add_library(hrc ${hrc_SRC})
target_compile_options(hrc PUBLIC -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror=implicit-function-declaration "$<$<CONFIG:DEBUG>:-Og;-ggdb3>" "$<$<CONFIG:RELEASE>:-O3>")
target_link_options(hrc PUBLIC "$<$<CONFIG:RELEASE>:-flto>")
