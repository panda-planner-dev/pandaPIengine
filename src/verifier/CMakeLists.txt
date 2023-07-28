project(verifier)


# commands for the util library
file(GLOB UTIL_SOURCE_FILES "util/src/*.cpp")

add_library(util
    ${UTIL_SOURCE_FILES}
        util/src/distribution.cpp)

target_include_directories(util
    PUBLIC
        ${pandaPIengine_SOURCE_DIR}
        util/include
)

target_link_libraries(util
    PUBLIC
        intDataStructures
)

add_library(sat_verifier
    	sat_verifier/src/sat_verifier.cpp
		sat_verifier/src/variables.cpp
		sat_verifier/src/constraints.cpp
		${pandaPIengine_SOURCE_DIR}/Model.cpp
		${pandaPIengine_SOURCE_DIR}/Util.cpp
		${pandaPIengine_SOURCE_DIR}/Debug.cpp
		${pandaPIengine_SOURCE_DIR}/ProgressionNetwork.cpp
		${pandaPIengine_SOURCE_DIR}/Invariants.cpp sat_verifier/src/depth.cpp sat_verifier/src/marker.cpp)

target_include_directories(sat_verifier
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/sat_verifier/include
        ${pandaPIengine_SOURCE_DIR}/
)

target_link_libraries(sat_verifier
    PUBLIC
        util
        intDataStructures
        sat
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

add_custom_target(cmd
    DEPENDS cmdline.h cmdline.c
)

file(GLOB VERIFIER_SOUCE_FILES "src/*.cpp")

add_executable(run_verifier ${VERIFIER_SOUCE_FILES} cmdline.c cmdline.h)

target_link_libraries(run_verifier
	PUBLIC
        sat_verifier
)

target_include_directories(run_verifier
	PUBLIC
		${PROJECT_SOURCE_DIR}
		${PROJECT_SOURCE_DIR}/include
)

add_subdirectory(test)
enable_testing()
add_executable(test_util
		${CMAKE_CURRENT_SOURCE_DIR}/test/test_util.cpp)
target_link_libraries(test_util
		PUBLIC
			test_main
			catch2
			util)

add_executable(test_depth
		${CMAKE_CURRENT_SOURCE_DIR}/test/test_depth.cpp
		${pandaPIengine_SOURCE_DIR}/ProgressionNetwork.cpp)

target_link_libraries(test_depth
		PUBLIC
			test_main
			catch2
			sat_verifier)
target_include_directories(test_depth
		PUBLIC
		${pandaPIengine_SOURCE_DIR}/)