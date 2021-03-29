file(GLOB search_SRC "*.h" "*.cpp" "fringes/*.h" "fringes/*.cpp")

add_library(search ${search_SRC})
target_compile_options(search PUBLIC ${compiler_flags})
target_link_options(search PUBLIC ${linker_flags})

if (CPLEX_SOURCE_DIR)
	add_library(cplex SHARED IMPORTED) 
	set_target_properties(cplex  PROPERTIES
	  IMPORTED_LOCATION "${CPLEX_SOURCE_DIR}/cplex/lib/x86-64_linux/static_pic/libcplex.a"
	  INTERFACE_INCLUDE_DIRECTORIES "${CPLEX_SOURCE_DIR}/cplex/include"
	)
	
	add_library(ilocplex SHARED IMPORTED) 
	set_target_properties(ilocplex  PROPERTIES
	  IMPORTED_LOCATION "${CPLEX_SOURCE_DIR}/cplex/lib/x86-64_linux/static_pic/libilocplex.a"
	  INTERFACE_INCLUDE_DIRECTORIES "${CPLEX_SOURCE_DIR}/cplex/include"
	)
	
	add_library(concert SHARED IMPORTED) 
	set_target_properties(concert  PROPERTIES
	  IMPORTED_LOCATION "${CPLEX_SOURCE_DIR}/concert/lib/x86-64_linux/static_pic/libconcert.a"
	  INTERFACE_INCLUDE_DIRECTORIES "${CPLEX_SOURCE_DIR}/concert/include"
	)
	
	
	target_link_libraries(search ilocplex)
	target_link_libraries(search concert)
	target_link_libraries(search cplex)
	target_link_libraries(search m)
	target_link_libraries(search pthread)
	target_link_libraries(search dl)
endif (CPLEX_SOURCE_DIR)

