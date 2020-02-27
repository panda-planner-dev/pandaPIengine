file(GLOB hlm_SRC "*.h" "*.cpp" "./lmDataStructures/*.cpp" "./lmDataStructures/*.h" "./lmExtraction/*.cpp" "./lmExtraction/*.h")

add_library(hlm ${hlm_SRC})
target_compile_options(hlm PUBLIC -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror=implicit-function-declaration "$<$<CONFIG:DEBUG>:-Og;-ggdb3>" "$<$<CONFIG:RELEASE>:-O3>")
target_link_options(hlm PUBLIC "$<$<CONFIG:RELEASE>:-flto>")
