project(verifier)


# commands for the util library
file(GLOB UTIL_SOURCE_FILES "util/src/*.cpp")

add_library(util
    ${UTIL_SOURCE_FILES}
)

target_include_directories(util
    PUBLIC
        ${pandaPIengine_SOURCE_DIR}
        util/include
)

target_link_libraries(util
    PUBLIC
        intDataStructures
)

# commands for the TO verifier
file(GLOB TO_SOURCE_FILES "to_verifier/src/*.cpp")

add_library(to_verifier
    ${TO_SOURCE_FILES}
)

target_include_directories(to_verifier
    PUBLIC
        ${PROJECT_SOURCE_DIR}/to_verifier/include
        ${pandaPIengine_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(to_verifier
    PUBLIC
        intDataStructures
)

# commands for command-line arguments
find_program(GENGETOPT gengetopt)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/cmdline.h ${PROJECT_SOURCE_DIR}/cmdline.c
	COMMAND gengetopt --include-getopt --default-optional --unamed-opts -i options.ggo
	DEPENDS options.ggo
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "Generating getopt parser code (cmdline.{h,c}) ..."
	VERBATIM
)