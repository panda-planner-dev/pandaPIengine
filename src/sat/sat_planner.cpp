#include "sat_planner.h"
#include "ipasir.h"
#include "pdt.h"

void solve_with_sat_planner(Model * htn){
	// start by determining whether this model is totally ordered
	cout << "Instance is totally ordered: " << htn->isTotallyOrdered() << endl;
	// compute transitive closures of all methods
	htn->computeTransitiveClosureOfMethodOrderings();
	htn->buildOrderingDatastructures();

	PDT* pdt = initialPDT(htn);
	expandPDTUpToLevel(pdt,5,htn);

	printPDT(htn,pdt);

	vector<PDT*> leafs;
	getLeafs(pdt,leafs);
	cout << "Number of leafs: " << leafs.size() << endl;
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
