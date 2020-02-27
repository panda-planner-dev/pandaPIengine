file(GLOB intDataStructures_SRC "*.h" "*.cpp")

add_library(intDataStructures ${intDataStructures_SRC})
target_compile_options(intDataStructures PUBLIC -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror=implicit-function-declaration "$<$<CONFIG:DEBUG>:-Og;-ggdb3>" "$<$<CONFIG:RELEASE>:-O3>")
target_link_options(intDataStructures PUBLIC "$<$<CONFIG:RELEASE>:-flto>")

