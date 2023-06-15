#include "state_formula.h"
#include "ipasir.h"
#include "../Invariants.h"
#include <cassert>
#include <iomanip>

using namespace std;

void generate_state_transition_formula(void* solver, sat_capsule & capsule, vector<vector<pair<int,int>>> & actionVariables, vector<int> & block_base_variables, Model* htn){
	// no blocks so just unit blocks
	vector<vector<int>> blocks;
	for (size_t time = 0; time < actionVariables.size(); time++){
		vector<int> block;
		block.push_back(time);
		blocks.push_back(block);
	}

	return generate_state_transition_formula(solver,capsule,actionVariables,block_base_variables,blocks,htn);
}
	
void generate_state_transition_formula(void* solver, sat_capsule & capsule, vector<vector<pair<int,int>>> & actionVariables, vector<int> & block_base_variables, vector<vector<int>> & blocks, Model* htn){

	cout << "Generating state formula for " << blocks.size() << " blocks." << endl;

	// as many blocks as we have timesteps
	int timesteps = blocks.size();
	block_base_variables.resize(timesteps);
	int goalBase = -1;
	/////////////////////// create variables for atoms per time step
	for (size_t time = 0; time <= timesteps; time++){
		// state variables at time
		assert(htn->numStateBits > 0);
		
		int base = capsule.new_variable();
		
		if (time < timesteps)
			block_base_variables[time] = base;     // save the variables in the first leaf of the block
		else
			goalBase = base;
		
		DEBUG(capsule.registerVariable(base,"state var " + pad_int(0) + " @ " + pad_int(time) + ": " + pad_string(htn->factStrs[0])));

		for (size_t svar = 1; svar < htn->numStateBits; svar++){
#ifndef NDEBUG
			int _r =
#endif
				capsule.new_variable(); // ignore return, they will be incremental
			assert(_r == base + svar);
			DEBUG(capsule.registerVariable(base + svar,	"state var " + pad_int(svar) + " @ " + pad_int(time) + ": " + pad_string(htn->factStrs[svar])));
		}
	}

	//////////////////////// state transition
	for (size_t time = 0; time < timesteps; time++){
		int beforeClauses = get_number_of_clauses();
#ifndef NDEBUG
		int bef = get_number_of_clauses();
#endif
		int thisTimeBase = block_base_variables[time];
		int nextTimeBase = (time == (timesteps - 1))?goalBase:block_base_variables[time+1];

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
	
		int afterClauses = get_number_of_clauses();
		//cout << "Timestep clauses " << afterClauses - beforeClauses << " from " << blocks[time].size() << " actions." << endl;
	}


	// assert initial state
	int base0 = block_base_variables[0];
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
}

void generate_mutex_formula(void* solver, sat_capsule & capsule, vector<int> & block_base_variables, unordered_set<int>* & after_leaf_invariants, Model* htn){
	// no blocks so just unit blocks
	vector<vector<int>> blocks;
	for (size_t time = 0; time < block_base_variables.size(); time++){
		vector<int> block;
		block.push_back(time);
		blocks.push_back(block);
	}

	generate_mutex_formula(solver, capsule, block_base_variables, blocks, after_leaf_invariants, htn);
}

	
void generate_mutex_formula(void* solver, sat_capsule & capsule, vector<int> & block_base_variables, vector<vector<int>> & blocks, 
		unordered_set<int>* & after_leaf_invariants, Model* htn){
	std::clock_t solver_start = std::clock();

	for (size_t time = 0; time < blocks.size(); time++){
		int timeBase = block_base_variables[time];

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

		
		//int lastTime = blocks[time].back();

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

void no_abstract_in_leaf(void* solver, vector<PDT*> & leafs, Model* htn){
	// forbid abstract tasks in leafs
	for (PDT* & leaf : leafs){
		for (size_t a = 0; a < leaf->possibleAbstracts.size(); a++)
			if (leaf->abstractVariable[a] != -1)
				assertNot(solver,leaf->abstractVariable[a]);
	}
}


void get_linear_state_atoms(sat_capsule & capsule, vector<PDT*> & leafs, vector<vector<pair<int,int>>> & ret){
	// these are just the primitives in the correct order

	for (PDT* & leaf : leafs){
#ifndef NDEBUG
		std::cout << "Position: " << ret.size() << " Leaf " << pad_path(leaf->path) << std::endl; 
#endif
		vector<pair<int,int>> atoms;
		for (size_t p = 0; p < leaf->possiblePrimitives.size(); p++)
			if (leaf->primitiveVariable[p] != -1)
				atoms.push_back(make_pair(leaf->primitiveVariable[p], leaf->possiblePrimitives[p]));
		
		ret.push_back(atoms);
	}
}

void get_partial_state_atoms(sat_capsule & capsule, Model * htn, SOG* sog, vector<vector<pair<int,int>>> & ret, bool effectLessActionsInSeparateLeafs ){
	// determine which leafs only contain method preconditions
	int leafsWithoutActualActions = 0;
	sog->leafContainsEffectAction.resize(sog->numberOfVertices);
	for (int t = 0; t < sog->numberOfVertices; t++){
		bool actualAction = false;
		bool effectLessAction = false;
		for (int prim : sog->leafOfNode[t]->possiblePrimitives){
			if (htn->numAdds[prim] > 0 || htn->numDels[prim] > 0){
				actualAction = true;
			} else {
				effectLessAction = true;
			}
		}

		if (effectLessActionsInSeparateLeafs && actualAction && effectLessAction) exit(0);
		if (!actualAction) leafsWithoutActualActions++;
		sog->leafContainsEffectAction[t] = actualAction;
	}
	int numberOfTimeSteps = sog->numberOfVertices - leafsWithoutActualActions;
	numberOfTimeSteps = numberOfTimeSteps;
	if (numberOfTimeSteps == 0) numberOfTimeSteps = 1;
	
	cout << "Total Leafs: " << sog->numberOfVertices << " of that without effect-actions " << leafsWithoutActualActions << " remaining: " << numberOfTimeSteps << endl;

	// determine to which times a given leaf can potentially be mapped
	sog->calcSucessorSets();
	sog->calcPredecessorSets();


	sog->firstPossible.resize(sog->numberOfVertices);
	sog->lastPossible.resize(sog->numberOfVertices);

	vector<unordered_set<int>> actionsPerPosition (numberOfTimeSteps);
	for (int t = 0; t < sog->numberOfVertices; t++){
		int numSucc = -1;
	   	for (int n : sog->successorSet[t])
			if (sog->leafContainsEffectAction[n])
				numSucc++;
		if (numSucc < 0) numSucc = 0;

		int numPrec = -1;
		for (int n : sog->predecessorSet[t])
			if (sog->leafContainsEffectAction[n])
				numPrec++;
		if (numPrec < 0) numPrec = 0;


		//int firstPossible = numPrec;
		//int lastPossible = numberOfTimeSteps - 1 - numSucc;
		int firstPossible = 0;
		int lastPossible = numberOfTimeSteps - 1;
		
		sog->firstPossible[t] = firstPossible;
		sog->lastPossible[t] = lastPossible;

#ifndef NDEBUG
		std::cout << "Position: " << t << " succ: " << numSucc << " prec: " << numPrec;
		std::cout << "\t\tfrom " << firstPossible << " to " << lastPossible << std::endl;
#endif

		for (int prim : sog->leafOfNode[t]->possiblePrimitives)
			for (int pos = firstPossible; pos <= lastPossible; pos++)
				actionsPerPosition[pos].insert(prim);

	}

/*	for (int t = 0; t < sog->numberOfVertices; t++){
		vector<pair<int,int>> atoms;
		for (size_t p = 0; p < htn->numActions; p++){
			int pvar = capsule.new_variable();
			DEBUG(capsule.registerVariable(pvar,"action var " + pad_int(p) + " @ " + pad_int(t) + ": " + pad_string(htn->taskNames[p])));
			atoms.push_back(make_pair(pvar, p));
		}
		
		ret.push_back(atoms);
	}
*/
	for (int t = 0; t < numberOfTimeSteps; t++){
		vector<pair<int,int>> atoms;
		for (int p : actionsPerPosition[t]){
			int pvar = capsule.new_variable();
			DEBUG(capsule.registerVariable(pvar,"action var " + pad_int(p) + " @ " + pad_int(t) + ": " + pad_string(htn->taskNames[p])));
			atoms.push_back(make_pair(pvar, p));
		}
		
		ret.push_back(atoms);
	}
}

