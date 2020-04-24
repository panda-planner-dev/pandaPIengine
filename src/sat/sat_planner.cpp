#include "sat_planner.h"
#include "sat_encoder.h"
#include "ipasir.h"
#include "pdt.h"

void solve_with_sat_planner(Model * htn){
	setDebugMode(true);

	// start by determining whether this model is totally ordered
	cout << "Instance is totally ordered: " << htn->isTotallyOrdered() << endl;
	// compute transitive closures of all methods
	htn->computeTransitiveClosureOfMethodOrderings();
	htn->buildOrderingDatastructures();

	PDT* pdt = new PDT(htn);
	pdt->expandPDTUpToLevel(5,htn);

#ifndef NDEBUG
	printPDT(htn,pdt);
#endif

	vector<PDT*> leafs;
	pdt->getLeafs(leafs);
	cout << "Number of leafs: " << leafs.size() << endl;


	sat_capsule capsule;

	pdt->assignVariableIDs(capsule, htn);

	DEBUG(capsule.printVariables());
	
	cout << ipasir_signature() << endl;
	void* solver = ipasir_init();
	pdt->addDecompositionClauses(solver, capsule);

	// assert the initial abstract task
	ipasir_add(solver,pdt->abstractVariable[0]);
	ipasir_add(solver,0);
	
	
	int state = ipasir_solve(solver);
	cout << "Solver state: " << state << endl;
	if (state == 10){
		for (int v = 1; v <= capsule.number_of_variables; v++){
			int val = ipasir_val(solver,v);
		
			std::string s = std::to_string(v);
			int x = 4 - s.size();
			while (x-- && x > 0) std::cout << " ";
			std::cout << v << ": ";
			if (val > 0) std::cout << "    ";
			else         std::cout << "not ";
#ifndef NDEBUG
			std::cout << capsule.variableNames[v] << endl; 
#else
			std::cout << v << endl;
#endif
		}
	}
}



void sat_solver_call(){
	cout << ipasir_signature() << endl;
	void* solver = ipasir_init();
	ipasir_add(solver,-1);
	ipasir_add(solver,-2);
	ipasir_add(solver,0);
	
	ipasir_add(solver,-3);
	ipasir_add(solver,2);
	ipasir_add(solver,0);
	
	ipasir_add(solver,3);
	ipasir_add(solver,0);

	int state = ipasir_solve(solver);
	cout << state << endl;
	if (state == 10){
		for (int v = 1; v <= 3; v++)
			cout  << "V " << v << ": " << ipasir_val(solver,v) << endl; 
	}
}
