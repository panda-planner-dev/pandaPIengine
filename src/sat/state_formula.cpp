#include "state_formula.h"
#include "ipasir.h"
#include <cassert>

void generate_state_transition_formula(void* solver, sat_capsule & capsule, vector<vector<pair<int,int>>> & actionVariables, vector<PDT*> & leafs, Model* htn){


	int goalBase = -1;
	/////////////////////// create variables for atoms per time step
	for (size_t time = 0; time <= actionVariables.size(); time++){
		// state variables at time
		assert(htn->numStateBits > 0);
		
		int base = capsule.new_variable();
		
		if (time < actionVariables.size())
			leafs[time]->baseStateVarVariable = base;
		else
			goalBase = base;
		
		DEBUG(capsule.registerVariable(base,"sate var " + pad_int(0) + " @ " + pad_int(time) + ": " + pad_string(htn->factStrs[0])));

		for (size_t svar = 1; svar < htn->numStateBits; svar++){
			int r = capsule.new_variable(); // ignore return, they will be incremental
			assert(r == base + svar);
			DEBUG(capsule.registerVariable(base + svar,	"sate var " + pad_int(svar) + " @ " + pad_int(time) + ": " + pad_string(htn->factStrs[svar])));
		}
	}

	//////////////////////// state transition
	for (size_t time = 0; time < actionVariables.size(); time++){
		int thisTimeBase = leafs[time]->baseStateVarVariable;
		int nextTimeBase = (time == (actionVariables.size() - 1))?goalBase:leafs[time+1]->baseStateVarVariable;

		vector<vector<int>> causingPositive (htn->numStateBits);
		vector<vector<int>> causingNegative (htn->numStateBits);

		// rules for the individual actions
		for (const auto & [varID,taskID] : actionVariables[time]){
			// preconditions must be fulfilled
			for (size_t precIndex = 0; precIndex < htn->numPrecs[taskID]; precIndex++)
				implies(solver, varID, thisTimeBase + htn->precLists[taskID][precIndex]);
			// add effects	
			for (size_t addIndex = 0; addIndex < htn->numAdds[taskID]; addIndex++){
				int add = htn->addLists[taskID][addIndex];
				implies(solver, varID, nextTimeBase + add);
				causingPositive[add].push_back(varID);
			}
			// del effects
			for (size_t delIndex = 0; delIndex < htn->numDels[taskID]; delIndex++){
				int del = htn->delLists[taskID][delIndex];
				impliesNot(solver, varID, nextTimeBase + del);
				causingNegative[del].push_back(varID);
			}
		}

	 	// frame axioms, state can only change due to an action
		for (size_t svar = 0; svar < htn->numStateBits; svar++){
			impliesPosAndNegImpliesOr(solver, nextTimeBase + svar, thisTimeBase + svar, causingPositive[svar]);
			impliesPosAndNegImpliesOr(solver, thisTimeBase + svar, nextTimeBase + svar, causingNegative[svar]);
		}
	}


	// assert initial state
	int base0 = leafs[0]->baseStateVarVariable;
	unordered_set<int> initSet;
	for (size_t i = 0; i < htn->s0Size; i++) initSet.insert(htn->s0List[i]);
	for (int svar = 0; svar < int(htn->numStateBits); svar++){
		if (initSet.count(svar))
			assertYes(solver,base0 + svar);
		else
			assertNot(solver,base0 + svar);
	}

	// assert goal state
	unordered_set<int> goalSet;
	for (size_t i = 0; i < htn->gSize; i++) goalSet.insert(htn->gList[i]);
	for (int svar = 0; svar < int(htn->numStateBits); svar++){
		if (goalSet.count(svar))
			assertYes(solver,goalBase + svar);
		else
			assertNot(solver,goalBase + svar);
	}


	for (PDT* & leaf : leafs){
		for (const int & abs : leaf->abstractVariable)
			assertNot(solver,abs);
	}


}

void get_linear_state_atoms(sat_capsule & capsule, vector<PDT*> & leafs, vector<vector<pair<int,int>>> & ret){
	// these are just the primitives in the correct order

	for (PDT* & leaf : leafs){
#ifndef NDEBUG
		std::cout << "Leaf " << pad_path(leaf->path) << std::endl; 
#endif
		// TODO assert order!
		vector<pair<int,int>> atoms;
		for (size_t p = 0; p < leaf->possiblePrimitives.size(); p++)
			atoms.push_back(make_pair(leaf->primitiveVariable[p], leaf->possiblePrimitives[p]));
		
		ret.push_back(atoms);
	}
}
