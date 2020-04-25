#include "sat_planner.h"
#include "sat_encoder.h"
#include "ipasir.h"
#include "pdt.h"
#include "state_formula.h"
#include <cassert>


void printSolution(void * solver, Model * htn, PDT* pdt){
	vector<PDT*> leafs;
	pdt->getLeafs(leafs);


	//for (size_t time = 0; time < leafs.size(); time++){
	//	int timeBase = leafs[time]->baseStateVarVariable;

	//	for (int v = 0; v < htn->numVars; v++){
	//		if (htn->firstIndex[v] == htn->lastIndex[v]) continue; // STRIPS
	//		
	//		std::set<int> tru;
	//		for (int f = htn->firstIndex[v]; f <= htn->lastIndex[v]; f++)
	//			if (ipasir_val(solver,timeBase + f) > 0)
	//				tru.insert(f);

	//		if (tru.size() != 1){
	//			cout << "Timestep " << time << endl;
	//			cout << "Variable " << v << " " << htn->varNames[v] << " is not a SAS+ group ..." << endl;
	//			cout << "True are:";
	//			for (int f : tru)
	//				cout << " " << f << " " << htn->factStrs[f];
	//			cout << endl;
	//			
	//			exit(0);
	//		}
	//	}
	//}


	
	int currentID = 0;
	
	cout << "==>" << endl;
	/// extract the primitive plan
	for (PDT* & leaf : leafs){
		for (size_t pIndex = 0; pIndex < leaf->primitiveVariable.size(); pIndex++){
			int prim = leaf->primitiveVariable[pIndex];
			if (ipasir_val(solver,prim) > 0){
				assert(leaf->outputID == -1);
				leaf->outputID = currentID++;
				std::cout << leaf->outputID << " " << htn->taskNames[leaf->possiblePrimitives[pIndex]] << endl;
			}
		}
	}

	// assign numbers to decompositions
	pdt->assignOutputNumbers(solver,currentID, htn);
	cout << "root " << pdt->outputID << endl;

	// out decompositions
	pdt->printDecomposition(htn);
	cout << "<==" << endl;
}

void printVariableTruth(void* solver, Model * htn, sat_capsule & capsule){
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


void createFormulaForDepth(void* solver, PDT* pdt, Model * htn, sat_capsule & capsule, int depth){
	pdt->expandPDTUpToLevel(depth,htn);

#ifndef NDEBUG
	printPDT(htn,pdt);
#endif
	
	pdt->assignVariableIDs(capsule, htn);
	DEBUG(capsule.printVariables());

	pdt->addDecompositionClauses(solver, capsule);

	// assert the initial abstract task
	assertYes(solver,pdt->abstractVariable[0]);

	// get leafs
	vector<PDT*> leafs;
	pdt->getLeafs(leafs);
	
	// generate primitive executability formula
	vector<vector<pair<int,int>>> vars;
	get_linear_state_atoms(capsule, leafs, vars);
	generate_state_transition_formula(solver, capsule, vars, leafs, htn);

#ifdef SAT_USEMUTEXES
	generate_mutex_formula(solver,capsule,leafs,htn);
#endif
}




void solve_with_sat_planner(Model * htn){
	htn->writeToPDDL("foo-d.hddl", "foo-p.hddl");
	setDebugMode(true);

	// start by determining whether this model is totally ordered
	cout << "Instance is totally ordered: " << htn->isTotallyOrdered() << endl;
	// compute transitive closures of all methods
	htn->computeTransitiveClosureOfMethodOrderings();
	htn->buildOrderingDatastructures();

	PDT* pdt = new PDT(htn);
	sat_capsule capsule;

	int depth = 1;
	while (true){
		void* solver = ipasir_init();
		cout << "Generating formula for depth " << depth << endl;
		createFormulaForDepth(solver,pdt,htn,capsule,depth);
		cout << "Formula has " << capsule.number_of_variables << " vars and " << get_number_of_clauses() << " clauses." << endl;
		
		cout << "Starting solver" << endl;
		std::clock_t solver_start = std::clock();
		int state = ipasir_solve(solver);
		std::clock_t solver_end = std::clock();
		double solver_time_in_ms = 1000.0 * (solver_end-solver_start) / CLOCKS_PER_SEC;
		cout << "Solver time: " << solver_time_in_ms << "ms" << endl;
		
		
		cout << "Solver state: " << (state==10?"SAT":"UNSAT") << endl;
		if (state == 10){
#ifndef NDEBUG
			printVariableTruth(solver, htn, capsule);
#endif
			printSolution(solver,htn,pdt);
			ipasir_release(solver);
			return;
		} else
			depth++;
		// release the solver	
		ipasir_release(solver);
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
