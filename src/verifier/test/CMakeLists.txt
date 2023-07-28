add_library(catch2 INTERFACE)

target_include_directories(catch2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/catch2)

add_library(test_main
        OBJECT
        ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/catch2/catch.hpp
        )

target_include_directories(test_main
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/catch2
        )

