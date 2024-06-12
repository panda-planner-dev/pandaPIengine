file(GLOB sat_SRC "*.h" "*.cpp")

add_library(sat
        ${sat_SRC}
        ${pandaPIengine_SOURCE_DIR}/Util.cpp
        ${pandaPIengine_SOURCE_DIR}/Debug.cpp
        ${pandaPIengine_SOURCE_DIR}/Invariants.cpp)

target_compile_options(sat PUBLIC ${compiler_flags})
target_link_options(sat PUBLIC ${linker_flags})
target_include_directories(sat
        PUBLIC
            ${pandaPIengine_SOURCE_DIR}/)

find_package(cryptominisat5)
target_link_libraries(sat ipasircryptominisat5)
