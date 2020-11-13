#include "state_formula.h"
#include "ipasir.h"
#include "../Invariants.h"
#include <cassert>
#include <iomanip>

void generate_state_transition_formula(void* solver, sat_capsule & capsule, vector<vector<pair<int,int>>> & actionVariables, vector<PDT*> & leafs, Model* htn){
	// no blocks so just unit blocks
	vector<vector<int>> blocks;
	for (size_t time = 0; time < actionVariables.size(); time++){
		vector<int> block;
		block.push_back(time);
		blocks.push_back(block);
	}

	return generate_state_transition_formula(solver,capsule,actionVariables,leafs,blocks,htn);
}
	
void generate_state_transition_formula(void* solver, sat_capsule & capsule, vector<vector<pair<int,int>>> & actionVariables, vector<PDT*> & leafs, vector<vector<int>> & blocks, Model* htn){

	// as many blocks as we have timesteps
	int timesteps = blocks.size();

	int goalBase = -1;
	/////////////////////// create variables for atoms per time step
	for (size_t time = 0; time <= timesteps; time++){
		// state variables at time
		assert(htn->numStateBits > 0);
		
		int base = capsule.new_variable();
		
		if (time < timesteps)
			leafs[blocks[time][0]]->baseStateVarVariable = base;     // save the variables in the first leaf of the block
		else
			goalBase = base;
		
		DEBUG(capsule.registerVariable(base,"sate var " + pad_int(0) + " @ " + pad_int(time) + ": " + pad_string(htn->factStrs[0])));

		for (size_t svar = 1; svar < htn->numStateBits; svar++){
			int _r = capsule.new_variable(); // ignore return, they will be incremental
			assert(_r == base + svar);
			DEBUG(capsule.registerVariable(base + svar,	"sate var " + pad_int(svar) + " @ " + pad_int(time) + ": " + pad_string(htn->factStrs[svar])));
		}
	}

	//////////////////////// state transition
	for (size_t time = 0; time < timesteps; time++){
#ifndef NDEBUG
		int bef = get_number_of_clauses();
#endif
		int thisTimeBase = leafs[blocks[time][0]]->baseStateVarVariable;
		int nextTimeBase = (time == (timesteps - 1))?goalBase:leafs[blocks[time+1][0]]->baseStateVarVariable;

		vector<vector<int>> causingPositive (htn->numStateBits);
		vector<vector<int>> causingNegative (htn->numStateBits);

#ifndef NDEBUG
		int act = 0;
#endif
		for (const int & l : blocks[time]){
			// rules for the individual actions
			for (const auto & [varID,taskID] : actionVariables[l]){
#ifndef NDEBUG
				act++;
#endif
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
		}
#ifndef NDEBUG
		int eff = get_number_of_clauses();
#endif

	 	// frame axioms, state can only change due to an action
		for (size_t svar = 0; svar < htn->numStateBits; svar++){
			impliesPosAndNegImpliesOr(solver, nextTimeBase + svar, thisTimeBase + svar, causingPositive[svar]);
			impliesPosAndNegImpliesOr(solver, thisTimeBase + svar, nextTimeBase + svar, causingNegative[svar]);
		}
#ifndef NDEBUG
		int frame = get_number_of_clauses();
		cout << setw(4) << time << " actions " << setw(7) << act << " " << setw(8) << eff-bef << " " << setw(8) << frame-eff << endl;
#endif
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
	for (size_t i = 0; i < htn->gSize; i++)
		assertYes(solver,goalBase + htn->gList[i]);


	for (PDT* & leaf : leafs){
		for (int a = 0; a < leaf->possibleAbstracts.size(); a++)
			if (leaf->abstractVariable[a] != -1)
				assertNot(solver,leaf->abstractVariable[a]);
	}
}

void generate_mutex_formula(void* solver, sat_capsule & capsule, vector<PDT*> & leafs, unordered_set<int>* & after_leaf_invariants, Model* htn){
	// no blocks so just unit blocks
	vector<vector<int>> blocks;
	for (size_t time = 0; time < leafs.size(); time++){
		vector<int> block;
		block.push_back(time);
		blocks.push_back(block);
	}

	generate_mutex_formula(solver, capsule, leafs, blocks, after_leaf_invariants, htn);
}

	
void generate_mutex_formula(void* solver, sat_capsule & capsule, vector<PDT*> & leafs, vector<vector<int>> & blocks, 
		unordered_set<int>* & after_leaf_invariants, Model* htn){
	std::clock_t solver_start = std::clock();

	for (size_t time = 0; time < blocks.size(); time++){
		int timeBase = leafs[blocks[time][0]]->baseStateVarVariable;

		for (int v = 0; v < htn->numVars; v++){
			if (htn->firstIndex[v] == htn->lastIndex[v]) continue; // STRIPS
			
			vector<int> vars;
			for (int f = htn->firstIndex[v]; f <= htn->lastIndex[v]; f++)
				vars.push_back(timeBase + f);

			atMostOne(solver,capsule,vars);
			atLeastOne(solver,capsule,vars);
		}

		for (int m = 0; m < htn->numStrictMutexes; m++){
			if (htn->strictMutexesSize[m] == 2) continue;
			vector<int> vars;
			for (int me = 0; me < htn->strictMutexesSize[m]; me++)
				vars.push_back(timeBase + htn->strictMutexes[m][me]);

			atMostOne(solver,capsule,vars);
			atLeastOne(solver,capsule,vars);
		}

		for (int m = 0; m < htn->numMutexes; m++){
			if (htn->mutexesSize[m] == 2) continue;
			vector<int> vars;
			for (int me = 0; me < htn->mutexesSize[m]; me++)
				vars.push_back(timeBase + htn->mutexes[m][me]);

			atMostOne(solver,capsule,vars);
		}


		for (int i = 0; i < htn->numInvariants; i++){
			if (htn->invariantsSize[i] == 2) continue;
			vector<int> vars;
			for (int ie = 0; ie < htn->invariantsSize[i]; ie++){
				int inv = htn->invariants[i][ie];
				if (inv < 0)
					vars.push_back(-(timeBase + (-inv-1)));
				else
					vars.push_back(timeBase + inv);
			}
			
			atLeastOne(solver,capsule,vars);
		}

		
		int lastTime = blocks[time].back();

		for (int i = 0; i < 2*htn->numStateBits; i++){
			int a = i - htn->numStateBits;
			int pa = a; if (pa < 0) pa = -pa-1;


			for (const int & b : binary_invariants[i]){
				int pb = b; if (pb < 0) pb = -pb-1;
				if (pa > pb) continue;
				
				vector<int> vars;
				if (a < 0) vars.push_back(-(timeBase + -a -1));
				else	   vars.push_back( timeBase + a);
				if (b < 0) vars.push_back(-(timeBase + -b -1));
				else	   vars.push_back( timeBase + b);
				atLeastOne(solver,capsule,vars);
			}

			for (const int & b : after_leaf_invariants[i]){
				int pb = b; if (pb < 0) pb = -pb-1;
				if (pa > pb) continue;
				
				vector<int> vars;
				if (a < 0) vars.push_back(-(timeBase + -a -1));
				else	   vars.push_back( timeBase + a);
				if (b < 0) vars.push_back(-(timeBase + -b -1));
				else	   vars.push_back( timeBase + b);
				atLeastOne(solver,capsule,vars);
			}
		}

	}
	std::clock_t solver_end = std::clock();
	double solver_time_in_ms = 1000.0 * (solver_end-solver_start) / CLOCKS_PER_SEC;
	cout << "Invar time: " << solver_time_in_ms << "ms";
	cout << " SM: " << htn->numStrictMutexes << " M: " << htn->numMutexes << " I: " << htn->numInvariants << " SI: " << count_invariants(htn) << endl;
}


void get_linear_state_atoms(sat_capsule & capsule, vector<PDT*> & leafs, vector<vector<pair<int,int>>> & ret){
	// these are just the primitives in the correct order

	for (PDT* & leaf : leafs){
#ifndef NDEBUG
		std::cout << "Position: " << ret.size() << " Leaf " << pad_path(leaf->path) << std::endl; 
#endif
		// TODO assert order!
		vector<pair<int,int>> atoms;
		for (size_t p = 0; p < leaf->possiblePrimitives.size(); p++)
			if (leaf->primitiveVariable[p] != -1)
				atoms.push_back(make_pair(leaf->primitiveVariable[p], leaf->possiblePrimitives[p]));
		
		ret.push_back(atoms);
	}
}

