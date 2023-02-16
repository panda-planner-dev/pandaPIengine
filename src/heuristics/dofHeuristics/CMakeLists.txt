file(GLOB ilp_heuristics_SRC "*.h" "*.cpp")

add_library(ilpheuristics ${ilp_heuristics_SRC})
target_compile_options(ilpheuristics  PUBLIC ${compiler_flags})
target_link_options(ilpheuristics  PUBLIC ${linker_flags})
target_link_libraries(ilpheuristics heuristics)
target_link_libraries(ilpheuristics intDataStructures)

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


target_link_libraries(ilpheuristics ilocplex)
target_link_libraries(ilpheuristics concert)
target_link_libraries(ilpheuristics cplex)
target_link_libraries(ilpheuristics m)
target_link_libraries(ilpheuristics pthread)
target_link_libraries(ilpheuristics dl)

target_link_libraries(ilpheuristics hlm)
