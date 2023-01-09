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
file(GLOB TO_SOURCE_FILES 
    "to_verifier/src/*.cpp" 
    "../Model.cpp" 
    "../ProgressionNetwork.cpp" 
    "../Util.cpp" 
    "../Debug.cpp" 
    "../Invariants.cpp"
)

add_library(to_verifier
    ${TO_SOURCE_FILES}
    ${pandaPIengine_SOURCE_DIR}/Model.cpp
    ${pandaPIengine_SOURCE_DIR}/ProgressionNetwork.cpp
    ${pandaPIengine_SOURCE_DIR}/Util.cpp
    ${pandaPIengine_SOURCE_DIR}/Debug.cpp
    ${pandaPIengine_SOURCE_DIR}/Invariants.cpp
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
        util
)

# commands for command-line arguments
# find_program(GENGETOPT gengetopt)

# add_custom_command(
# 	OUTPUT ${PROJECT_SOURCE_DIR}/cmdline.h ${PROJECT_SOURCE_DIR}/cmdline.c
# 	COMMAND gengetopt --include-getopt --default-optional --unamed-opts -i options.ggo
# 	DEPENDS options.ggo
# 	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
# 	COMMENT "Generating getopt parser code (cmdline.{h,c}) ..."
# 	VERBATIM
# )

