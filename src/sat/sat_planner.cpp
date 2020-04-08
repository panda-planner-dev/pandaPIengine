#include "sat_planner.h"
#include "ipasir.h"
#include "pdt.h"

void solve_with_sat_planner(Model * htn){
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


	PDT* pdt = initialPDT(htn);
	expandPDT(pdt,htn);
}
