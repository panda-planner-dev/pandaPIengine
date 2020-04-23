#include "sat_encoder.h"
#include "ipasir.h"
#include <iostream>
#include <cassert>

static bool debugMode = false;


bool getDebugMode (void)
{
	return debugMode;
}

void setDebugMode (bool enabled)
{
#ifndef NDEBUG
	debugMode = enabled;
#else
	if (enabled)
	{
		std::cerr << "Tried to enable debug mode, but the program was built with debugging disabled." << std::endl;
	}
#endif
}

sat_capsule::sat_capsule(){
	number_of_variables = 0;
}

int sat_capsule::new_variable(){
	return ++number_of_variables;
}

#ifndef NDEBUG
void sat_capsule::registerVariable(int v, std::string name){
	assert(variableNames.count(v) == 0);
	variableNames[v] = name;	
}

void sat_capsule::printVariables(){
	for (auto & p : variableNames){
		std::string s = std::to_string(p.first);
		int x = 4 - s.size();
		while (x--) std::cout << " ";
		std::cout << s << " -> " << p.second << std::endl;
	}
}


void implies(void* solver, int i, int j){
	//DEBUG(std::cout << "Adding " << -i << " " << j << " " << 0 << std::endl);
	ipasir_add(solver,-i);
	ipasir_add(solver,j);
	ipasir_add(solver,0);
}

void impliesOr(void* solver, int i, std::vector<int> & j){
	ipasir_add(solver,-i);
	for (int & x : j)
		ipasir_add(solver,x);
	ipasir_add(solver,0);
}

#endif
