#include "sat_planner.h"
#include "sat_encoder.h"
#include "ipasir.h"
#include "pdt.h"
#include "state_formula.h"
#include "disabling_graph.h"
#include "partial_order.h"
#include "../Util.h"
#include "../Invariants.h"
#include <cassert>
#include <thread> 
#include <chrono>
#include <pthread.h>
#include <signal.h>
#include <fstream>
#include <iomanip>
#include <cmath>

#include <sys/sysinfo.h>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


bool optimisePlan = false;
bool costMode = false;
int currentMaximumCost;
int costFactor = 1000;
bool useLogCostsForOptimisation = true;

int* originalActionCosts;


pair<int,int> printSolution(void * solver, Model * htn, PDT* pdt, MatchingData & matching){
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
	int solutionCost = 0;
	int trueSolutionCost = 0;
	
	cout << "==>" << endl;
	/// extract the primitive plan
	
	if (htn->isTotallyOrdered){
		for (PDT* & leaf : leafs){
			for (size_t pIndex = 0; pIndex < leaf->possiblePrimitives.size(); pIndex++){
				int prim = leaf->primitiveVariable[pIndex];
				if (prim == -1) continue;
				if (ipasir_val(solver,prim) > 0){
					assert(leaf->outputID == -1);
					leaf->outputID = currentID++;
					std::cout << leaf->outputID << " " << htn->taskNames[leaf->possiblePrimitives[pIndex]] << endl;
					solutionCost += htn->actionCosts[leaf->possiblePrimitives[pIndex]];
					trueSolutionCost += originalActionCosts[leaf->possiblePrimitives[pIndex]];
#ifndef NDEBUG
					cout << "Assigning " << leaf->outputID << " to atom " << prim << endl;
#endif
				}
			}
		}
	}

	// assign numbers to decompositions
	pdt->assignOutputNumbers(solver,currentID, htn);
	
	// if po output the primitive plan now
	if (!htn->isTotallyOrdered){
		for (int p = 0; p < matching.matchingPerPosition.size(); p++){
			for (auto [pvar, prim] : matching.vars[p]){
				if (ipasir_val(solver,pvar) > 0){
					// find the if of the matched leaf
					for (int l = 0; l < matching.matchingPerPosition[p].size(); l++){
						if (ipasir_val(solver,matching.matchingPerPosition[p][l]) > 0
								&& matching.leafSOG->leafOfNode[l]->outputTask == prim){
							// get the output number of that leaf
							PDT * leaf = matching.leafSOG->leafOfNode[l];
							std::cout << p << "@" << l << " | " << leaf->outputID << " " << htn->taskNames[prim] << endl;
							cout << "PUP" << endl;
						}
					}
					
				}
			}
		}
	}
	
		
	cout << "root " << pdt->outputID << endl;

	// out decompositions
	pdt->printDecomposition(htn);
	cout << "<==" << endl;
	cout << "Total cost of solution: " << solutionCost << " true cost: " << trueSolutionCost << endl;
	return {solutionCost,trueSolutionCost};
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





bool filter_leafs_ff(vector<PDT*> & leafs, Model * htn){
	bool * reachableFacts = new bool[htn->numStateBits];
	for (int i = 0; i < htn->numStateBits; i++)
		reachableFacts[i] = false;
	for (int s0Index = 0; s0Index < htn->s0Size; s0Index++)
		reachableFacts[htn->s0List[s0Index]] = true;


	int executablePrimitives = 0;
	int prunedPrimitives = 0;
	for (unsigned int l = 0; l < leafs.size(); l++){
		PDT* leaf = leafs[l];

		vector<int> executable;
		for (unsigned int primI = 0; primI < leaf->possiblePrimitives.size(); primI++){
			if (leaf->prunedPrimitives[primI]) continue; // is already pruned
			int prim = leaf->possiblePrimitives[primI];
			bool isExecutable = true;
			for (int prec = 0; isExecutable && prec < htn->numPrecs[prim]; prec++)
				isExecutable = reachableFacts[htn->precLists[prim][prec]];

			if (isExecutable) {
				executable.push_back(prim);
				executablePrimitives++;
				//cout << "    Executable at " << l << " prim: " << prim << " " << htn->taskNames[prim] << endl;
			} else {
				leaf->prunedPrimitives[primI] = true;
				prunedPrimitives++;
				//cout << "Not executable at " << l << " prim: " << prim << " " << htn->taskNames[prim] << endl;
			}
		}


		for (int prim : executable)
			for (int add = 0; add < htn->numAdds[prim]; add++)
				reachableFacts[htn->addLists[prim][add]] = true;
	}

	cout << "FF Pruning: removed " << prunedPrimitives << " of " << (prunedPrimitives + executablePrimitives) << endl;
	return prunedPrimitives != 0;
}


void insert_invariant(Model * htn, unordered_set<int> * invariants, int a, int b){
	if (binary_invariants[a + htn->numStateBits].count(b)) return; // is a global invariant

	invariants[a + htn->numStateBits].insert(b);
	invariants[b + htn->numStateBits].insert(a);
}


bool filter_leafs_Rintanen(vector<PDT*> & leafs, Model * htn, unordered_set<int>* & after_leaf_invariants, int & additionalInvariants){
	//std::clock_t invariant_start = std::clock();
	//cout << endl << "Computing invariants [Rintanen]" << endl;
	
	vector<pair<int,int>> v0;
	bool * toDelete;
	vector<vector<int>> posInvarsPerPredicate;
	vector<vector<int>> negInvarsPerPredicate;

	compute_Rintanen_initial_invariants(htn,v0,toDelete,posInvarsPerPredicate,negInvarsPerPredicate);
	
	
	int executablePrimitives = 0;
	int prunedPrimitives = 0;
	for (unsigned int l = 0; l < leafs.size(); l++){
		PDT* leaf = leafs[l];

		vector<int> executable;
		vector<pair<bool*,bool*>> inferredPreconditions;
		for (unsigned int primI = 0; primI < leaf->possiblePrimitives.size(); primI++){
			if (leaf->prunedPrimitives[primI]) continue; // is already pruned
			int prim = leaf->possiblePrimitives[primI];

			bool * posInferredPreconditions = new bool[htn->numStateBits];
			bool * negInferredPreconditions = new bool[htn->numStateBits];

			bool isExecutable = 
				compute_Rintanten_action_applicable(htn,prim,v0,toDelete, posInvarsPerPredicate, negInvarsPerPredicate, posInferredPreconditions, negInferredPreconditions);


			if (isExecutable) {
				executable.push_back(prim);
				inferredPreconditions.push_back(make_pair(posInferredPreconditions,negInferredPreconditions));
				executablePrimitives++;
				//cout << "    Executable at " << l << " prim: " << prim << " " << htn->taskNames[prim] << endl;
			} else {
				leaf->prunedPrimitives[primI] = true;
				delete[] posInferredPreconditions;
				delete[] negInferredPreconditions;
				prunedPrimitives++;
				//cout << "Not executable at " << l << " prim: " << prim << " " << htn->taskNames[prim] << endl;
			}
		}


		for (size_t i = 0; i < executable.size(); i++){
			int prim = executable[i];
			bool * posInferredPreconditions = inferredPreconditions[i].first;
			bool * negInferredPreconditions = inferredPreconditions[i].second;

			compute_Rintanten_action_effect(htn,prim,v0,toDelete, posInvarsPerPredicate, negInvarsPerPredicate, posInferredPreconditions, negInferredPreconditions);
		}
		// reduce data structures	
		compute_Rintanen_reduce_invariants(htn, v0, toDelete, posInvarsPerPredicate, negInvarsPerPredicate);
	}
	
	// old invariants are always ok, so don't clear, just add
	for (auto [a,b] : v0)
		insert_invariant(htn,after_leaf_invariants,a,b);

	additionalInvariants = v0.size();
	cout << "Rintanen Pruning: removed " << prunedPrimitives << " of " << (prunedPrimitives + executablePrimitives) << endl;
	return prunedPrimitives != 0;
}



bool createFormulaForDepth(void* solver, PDT* pdt, Model * htn, sat_capsule & capsule, MatchingData & matching, int depth,
		bool block_compression, bool sat_mutexes, sat_pruning pruningMode, bool effectLessActionsInSeparateLeaf)  {
	std::clock_t beforePDT = std::clock();
	pdt->expandPDTUpToLevel(depth,htn,effectLessActionsInSeparateLeaf);
	std::clock_t afterPDT = std::clock();
	double pdt_time = 1000.0 * (afterPDT - beforePDT) / CLOCKS_PER_SEC;
	cout << "Computing PDT took: " << setprecision(3) << pdt_time << " ms" << endl;
	//printMemory();
	// get leafs
	cout << "Computed PDT. Extracting leafs ... ";
	vector<PDT*> leafs;
	pdt->getLeafs(leafs);
	cout << leafs.size() << " leafs" << endl;

	SOG* leafsog = nullptr;
	if (!htn->isTotallyOrdered){
		cout << "Extracting leaf SOG ... ";
		leafsog = pdt->getLeafSOG();
		cout << "done" << endl;
	}
	
	/*
	ofstream dfile;
	dfile.open ("leafsog.dot");
	dfile << " digraph graphname" << endl << "{" << endl;
	leafsog->printDot(htn,dfile);
	dfile << "}" << endl;
	dfile.close();
	*/


	/*ofstream dfile;
	dfile.open ("pdt_" + to_string(depth) + ".dot");
	dfile << " digraph graphname" << endl << "{" << endl;
	pdt->printDot(htn,dfile);
	dfile << "}" << endl;
	dfile.close();*/
	
	
	//printMemory();
	cout << "Clear pruning tables ...";
	pdt->resetPruning(htn); // clear tables in whole tree
	cout << " done." << endl;
	//printPDT(htn,pdt);
	//printMemory();


	unordered_set<int>* after_leaf_invariants = new unordered_set<int>[2*htn->numStateBits];

	if (htn->isTotallyOrdered){
		// mark all abstracts in leafs as pruned
		for (PDT* l : leafs)
			for (size_t a = 0; a < l->prunedAbstracts.size(); a++)
				l->prunedAbstracts[a] = true;
		
		for (PDT* leaf : leafs) leaf->propagatePruning(htn);


		int pruningPhase = 1;
		int round = 1;
		int additionalInvariants = 0;
		while(pruningMode){
			cout << color(Color::BLUE,"Pruning round ") << round++ << " Phase: " << pruningPhase << endl;
			if (pruningPhase == 1){
				if (!filter_leafs_ff(leafs, htn)){
					if (pruningMode == pruningPhase)
						break;
					else pruningPhase++;
				}
			} else if (pruningPhase == 2){
				if (!filter_leafs_Rintanen(leafs, htn, after_leaf_invariants, additionalInvariants))
					break;
			}
		}
		for (PDT* leaf : leafs) leaf->propagatePruning(htn);

		int overallAssignments = 0;
		int prunedAssignments = 0;
		pdt->countPruning(overallAssignments, prunedAssignments, false);
		cout << "Pruning: " << prunedAssignments << " of " << overallAssignments << endl;
		overallAssignments = 0;
		prunedAssignments = 0;
		for (PDT* leaf : leafs)
			leaf->countPruning(overallAssignments, prunedAssignments, true);
		cout << "Leaf Primitive Pruning: " << prunedAssignments << " of " << overallAssignments << endl;

		// if we have pruned the initial abstract task, return ...	
		if (pdt->prunedAbstracts[0]) return false;
		
		cout << "Pruning gave " << additionalInvariants << " new invariants" << endl;	
		//printMemory();
	}

#ifndef NDEBUG
	printPDT(htn,pdt);
#endif
	/////////////////////////// generate the formula
	int numVarsBefore = capsule.number_of_variables;
	cout << "Assigning variable IDs for PDT ...";
	pdt->assignVariableIDs(capsule, htn);
	cout << " done. " << (capsule.number_of_variables - numVarsBefore) << " new variables." << endl;
	//printMemory();
	DEBUG(capsule.printVariables());
	
	//exit(0);

	int beforeDecomp = get_number_of_clauses();
	pdt->addDecompositionClauses(solver, capsule, htn);
	int afterDecomp = get_number_of_clauses();
	// assert the initial abstract task
	assertYes(solver,pdt->abstractVariable[0]);
	if (!htn->isTotallyOrdered)
		no_abstract_in_leaf(solver,leafs,htn);
	cout << "Decomposition Clauses generated." << endl;	
	
	pdt->addPrunedClauses(solver);
	//for (PDT* leaf : leafs) leaf->addPrunedClauses(solver); // add assertNo for pruned things
	cout << "Pruned clauses." << endl;	
	//printMemory();

	vector<vector<int>> blocks;

	if (htn->isTotallyOrdered && block_compression){
		blocks = compute_block_compression(htn, leafs);
		cout << "Block compression leads to " << blocks.size() << " timesteps." << endl;
#ifndef NDEBUG
		for (auto block : blocks){
			cout << endl << "New Block" << endl;
			for (int l : block){
				cout << "\tLeaf: " << path_string(leafs[l]->path) << endl;
				int i = 0;
				for (int a : leafs[l]->possiblePrimitives){
					if (!leafs[l]->prunedPrimitives[i])
						cout << "\t\t" << htn->taskNames[a] << endl;
					i++;
				}
			}
		}
#endif
	}
	
	cout << "Decomp formula generated" << endl;
	//printMemory();

	// generate primitive executability formula
	vector<vector<pair<int,int>>> vars;
	
	
	if (htn->isTotallyOrdered){
		get_linear_state_atoms(capsule, leafs, vars);
	} else {
		get_partial_state_atoms(capsule, htn, leafsog, vars, effectLessActionsInSeparateLeaf);
	}
	
	cout << "State atoms" << endl;

	vector<int> block_base_variables;

	if (htn->isTotallyOrdered && block_compression)
		generate_state_transition_formula(solver, capsule, vars, block_base_variables, blocks, htn);
	else
		generate_state_transition_formula(solver, capsule, vars, block_base_variables, htn);
	
	int afterState = get_number_of_clauses();
	cout << "State formula" << endl;

	if (!htn->isTotallyOrdered){
		generate_matching_formula(solver, capsule, htn, leafsog, vars, matching);
	}
	//printMemory();

	cout << "Matching" << endl;
	DEBUG(capsule.printVariables());

	if (sat_mutexes){
		if (htn->isTotallyOrdered && block_compression)
			generate_mutex_formula(solver, capsule, block_base_variables, blocks, after_leaf_invariants, htn);
		else
			generate_mutex_formula(solver, capsule, block_base_variables, after_leaf_invariants, htn);
	}
	
	int afterMutex = get_number_of_clauses();

	cout << color(Color::BLUE,"Formula: ") << (afterDecomp - beforeDecomp) << " decomposition " << (afterState - afterDecomp) << " state "  << (afterMutex - afterState) << " mutex" << endl;


	if (costMode){
		vector<int> costVars;
		for (int t = 0; t < vars.size(); t++){
			// maximum cost per time step
			int maxCost = 0;
			for (const auto & [varID,taskID] : vars[t])
				maxCost = max(maxCost,htn->actionCosts[taskID]);
			//cout << "\tMaximum Cost Time " << t << " is " << maxCost << endl;

			maxCost /= costFactor;

			//cout << "\tGenerating cost Implications" << endl;
			for (int c = 1; c <= maxCost; c++){
				//cout << "\tC=" << c << endl;
				int cvar = capsule.new_variable();
				DEBUG(capsule.registerVariable(cvar,"cost var " + pad_int(c) + " @ " + pad_int(t)));
				for (const auto & [varID,taskID] : vars[t])
					if (htn->actionCosts[taskID]/costFactor >= c)
						implies(solver,varID,cvar);
					else
						impliesNot(solver,varID,cvar);

				costVars.push_back(cvar);
			}
		}

		cout << "Generating at most K formula " << currentMaximumCost/costFactor << "*" << costVars.size() << "=" << currentMaximumCost/costFactor * costVars.size() <<  "... " << endl;
		atMostK(solver,capsule,currentMaximumCost/costFactor,costVars);
		cout << "done" << endl;
	}




	//map<int,string> names;
	//for (int i = 0; i < htn->numActions; i++)
	//	names[i] = htn->taskNames[i];

/*	
	map<int,string> style;
	for (int prim : leafs[0]->possiblePrimitives)
		style[prim] = "style=filled,fillcolor=green";

	for (int prim : leafs[1]->possiblePrimitives){
		if (style.count(prim))
			style[prim] = "style=filled,fillcolor=red";
		else
			style[prim] = "style=filled,fillcolor=blue";
	}
	
	for (int prim : leafs[2]->possiblePrimitives){
		if (style.count(prim))
			style[prim] = "style=filled,fillcolor=red";
		else
			style[prim] = "style=filled,fillcolor=yellow";
	}
	for (int prim : leafs[3]->possiblePrimitives){
		if (style.count(prim))
			style[prim] = "style=filled,fillcolor=red";
		else
			style[prim] = "style=filled,fillcolor=orange";
	}
	
	for (int prim : leafs[4]->possiblePrimitives){
		if (style.count(prim))
			style[prim] = "style=filled,fillcolor=red";
		else
			style[prim] = "style=filled,fillcolor=brown";
	}
	
	for (int prim : leafs[5]->possiblePrimitives){
		if (style.count(prim))
			style[prim] = "style=filled,fillcolor=red";
		else
			style[prim] = "style=filled,fillcolor=gray";
	}*/
	
	/*ofstream out("dg.dot");
    out << dg->dot_string(names);
    //out << dg->dot_string(names,style);
    out.close();
	system("dot -Tpdf dg.dot > dg.pdf");*/

	return true;	
}

namespace std {
template <> struct hash<std::pair<int, int>> {
    inline size_t operator()(const std::pair<int, int> &v) const {
        std::hash<int> int_hasher;
        return int_hasher(v.first) ^ int_hasher(v.second);
    }
};

}


void bdfs(Model * htn, PDT * cur, PDT * source, vector<pair<int,int>> possibleAssignments, map<PDT*, vector<pair<int,int>>> & overallAssignments){
	overallAssignments[cur] = possibleAssignments;
	/*cout << "\t\t" << cur;
	for (auto [task,method] : possibleAssignments)
		cout << " (" << task << "," << method << ")";
	cout << endl;*/

	// only propagate to the children, if we have actually computed some already ...
	if (cur->expanded){
		// we know that for cur the possibleAssignments are possible
		// the assignments are pairs of present task and applied method
		
		// determine what this can imply for all the children
		vector<unordered_set<pair<int,int>>> childrenPossibleAssignments (cur->children.size());

		for (auto [tIndex,mIndex] : possibleAssignments){

			if (tIndex != -1){
				// applying method mIndex, which tasks will this result in
				//assert(cur->listIndexOfChildrenForMethods.size() > tIndex);
				//assert(cur->listIndexOfChildrenForMethods[tIndex].size() > mIndex);
				for (size_t child = 0; child < cur->children.size(); child++){
					if (!cur->getListIndexOfChildrenForMethods(tIndex,mIndex,child)->present) continue;
					bool isPrimitive = cur->getListIndexOfChildrenForMethods(tIndex,mIndex,child)->isPrimitive;
					int subIndex = cur->getListIndexOfChildrenForMethods(tIndex,mIndex,child)->taskIndex;
					
					if (isPrimitive)
						childrenPossibleAssignments[child].insert(make_pair(-1, subIndex));
					else{
						assert(subIndex < cur->children[child]->possibleAbstracts.size());
						int & t = cur->children[child]->possibleAbstracts[subIndex];
						for (int m = 0; m < htn->numMethodsForTask[t]; m++)
							childrenPossibleAssignments[child].insert(make_pair(subIndex, m));
					}
				}
			} else {
				// inherited primitive, implies just one inheritence
				auto [child, tIndex,_] = cur->positionOfPrimitivesInChildren[mIndex]; // actually mIndex is the task
				childrenPossibleAssignments[child].insert(make_pair(-1,tIndex));
			}
		}

		for (size_t child = 0; child < cur->children.size(); child++){
			if (cur->children[child] == source) continue; // this is the task we come from
			vector<pair<int,int>> vec;
			for (auto & p : childrenPossibleAssignments[child])
				vec.push_back(p);
			bdfs(htn, cur->children[child], cur, vec, overallAssignments);
		}
	}


	// we have a parent task
	if (cur->mother != nullptr && cur->mother != source){
		// set for duplicate elimination
		unordered_set<pair<int,int>> possibleMotherAssignments;
		for (auto [tIndex,mIndex] : possibleAssignments){

			if (tIndex != -1){
				for (int c = 0; c < cur->numberOfCausesPerAbstract[tIndex]; c++){
					pair<int,int> pp;
					pp.first = cur->getCauseForAbstract(tIndex,c)->taskIndex;
					pp.second = cur->getCauseForAbstract(tIndex,c)->methodIndex;
					
					possibleMotherAssignments.insert(pp);
				}
			} else {
				for (int c = 0; c < cur->numberOfCausesPerPrimitive[mIndex]; c++){
					pair<int,int> pp;
					pp.first = cur->getCauseForAbstract(mIndex,c)->taskIndex;
					pp.second = cur->getCauseForAbstract(mIndex,c)->methodIndex;
					
					possibleMotherAssignments.insert(pp);
				}
			}
		}
		
		// push to mother
		vector<pair<int,int>> vec;
		for (auto & p : possibleMotherAssignments)
			vec.push_back(p);
		bdfs(htn, cur->mother, cur, vec, overallAssignments);
	}
}



void temp(Model * htn, PDT * pdt){
	vector<PDT*> leafs;
	pdt->getLeafs(leafs);

	for (PDT* l : leafs){
		for (size_t pI = 0; pI < l->possiblePrimitives.size(); pI++ ){
			int p = l->possiblePrimitives[pI];
			cout << "Leaf " << l << " " << p << endl;
			map<PDT*,vector<pair<int,int>>> overallAssignments;
			
			
			vector<pair<int,int>> possibleAssignments;
			for (int c = 0; c < l->numberOfCausesPerPrimitive[pI]; c++){
				pair<int,int> pp;
				pp.first = l->getCauseForPrimitive(pI,c)->taskIndex;	
				pp.second = l->getCauseForPrimitive(pI,c)->methodIndex;	
			}


			bdfs(htn, l->mother, l, possibleAssignments, overallAssignments);
			cout << "  Computed implications for " << overallAssignments.size() << " other vertices." << endl;
			cout << "  extracting mutexes" << endl;
			for (auto & [node,possible] : overallAssignments){
				for (size_t pIndex = 0; pIndex < node->possiblePrimitives.size(); pIndex++){
					// check
					bool canBeAssigned = false;
					for (auto [tIndex,mIndex] : possible) canBeAssigned |= (tIndex == -1) && (mIndex == pIndex);
					if (canBeAssigned) {
						cout << "    not mutex "<< node << " with " << pIndex << endl;
						continue;
					}
					cout << "        mutex "<< node <<" with " << pIndex << endl;
				}
			}
		}
	}
}



void optimise_with_sat_planner_linear_bound_increase(Model * htn, bool block_compression, bool sat_mutexes, sat_pruning pruningMode, bool effectLessActionsInSeparateLeaf,
		sat_capsule & capsule,
		PDT* pdt,
		int depth,
		int currentCost
		){

	pdt = new PDT(htn);

	costMode = true;
	currentMaximumCost = currentCost - 1;
	int lastImprovedDepth = depth;

	while (true){
		void* solver = ipasir_init();
		cout << endl << endl << color(Color::YELLOW, "Generating formula for depth " + to_string(depth) + " and cost " + to_string(currentMaximumCost)) << endl;
		std::clock_t formula_start = std::clock();
		int state = 20;

		MatchingData matching;
		if (createFormulaForDepth(solver,pdt,htn,capsule,matching,depth,block_compression,sat_mutexes, pruningMode, effectLessActionsInSeparateLeaf)){
			std::clock_t formula_end = std::clock();
			double formula_time_in_ms = 1000.0 * (formula_end-formula_start) / CLOCKS_PER_SEC;
			cout << "Formula has " << capsule.number_of_variables << " vars and " << get_number_of_clauses() << " clauses." << endl;
			cout << "Formula time: " << fixed << formula_time_in_ms << "ms" << endl;
			
			
			cout << "Starting solver" << endl;
			std::clock_t solver_start = std::clock();
			state = ipasir_solve(solver);
			std::clock_t solver_end = std::clock();
			double solver_time_in_ms = 1000.0 * (solver_end-solver_start) / CLOCKS_PER_SEC;
			cout << "Solver time: " << fixed << solver_time_in_ms << "ms" << endl;
			
			
			cout << "Solver state: " << color((state==10?Color::GREEN:Color::RED), (state==10?"SAT":"UNSAT")) << endl;
			//if (depth == 3) exit(0);
		} else {
			cout << "Initial abstract task is pruned: " <<  color(Color::RED,"UNSAT") << endl;
		}

		if (state == 10){
#ifndef NDEBUG
			printVariableTruth(solver, htn, capsule);
#endif
			auto [apparentCost,trueCost] = printSolution(solver,htn,pdt,matching);
			ipasir_release(solver);
			if (apparentCost < 1000)
				costFactor = 1; // try to get a better resolution ...
			currentMaximumCost = apparentCost - costFactor;
			lastImprovedDepth = depth;
			pdt = new PDT(htn);
		} else {
			depth++;
			ipasir_release(solver);

			// if we stay too long on this depth, try smaller steps
			if (depth + 15 < lastImprovedDepth){
				depth = lastImprovedDepth;
				costFactor = 1;
				currentMaximumCost += 999; // reset to a higher cost level
			}
		}
	}
}





void solve_with_sat_planner_linear_bound_increase(Model * htn, bool block_compression, bool sat_mutexes, sat_pruning pruningMode, bool effectLessActionsInSeparateLeaf){
	PDT* pdt = new PDT(htn);
	//graph * dg = compute_disabling_graph(htn, true);
	sat_capsule capsule;
	reset_number_of_clauses();

	int depth = 1;
	while (true){
		void* solver = ipasir_init();
		cout << endl << endl << color(Color::YELLOW, "Generating formula for depth " + to_string(depth)) << endl;
		std::clock_t formula_start = std::clock();
		int state = 20;

		MatchingData matching;
		if (createFormulaForDepth(solver,pdt,htn,capsule,matching,depth,block_compression,sat_mutexes, pruningMode, effectLessActionsInSeparateLeaf)){
			std::clock_t formula_end = std::clock();
			double formula_time_in_ms = 1000.0 * (formula_end-formula_start) / CLOCKS_PER_SEC;
			cout << "Formula has " << capsule.number_of_variables << " vars and " << get_number_of_clauses() << " clauses." << endl;
			cout << "Formula time: " << fixed << formula_time_in_ms << "ms" << endl;
			
			
			cout << "Starting solver" << endl;
			std::clock_t solver_start = std::clock();
			state = ipasir_solve(solver);
			std::clock_t solver_end = std::clock();
			double solver_time_in_ms = 1000.0 * (solver_end-solver_start) / CLOCKS_PER_SEC;
			cout << "Solver time: " << fixed << solver_time_in_ms << "ms" << endl;
			
			
			cout << "Solver state: " << color((state==10?Color::GREEN:Color::RED), (state==10?"SAT":"UNSAT")) << endl;
			//if (depth == 3) exit(0);
		} else {
			cout << "Initial abstract task is pruned: " <<  color(Color::RED,"UNSAT") << endl;
		}
		//temp(htn,pdt);
	
		if (state == 10){
#ifndef NDEBUG
			printVariableTruth(solver, htn, capsule);
#endif
			auto [apparentCost,trueCost] = printSolution(solver,htn,pdt,matching);
			ipasir_release(solver);
			if (optimisePlan)
				optimise_with_sat_planner_linear_bound_increase(htn, block_compression, sat_mutexes, pruningMode, effectLessActionsInSeparateLeaf, capsule, pdt, depth, apparentCost);
			return;
		} else {
			depth++;
			//return;
		}
		// release the solver	
		ipasir_release(solver);
	}
}

#define THREAD_PREFIX "\t\t\t\t\t\t\t\t\t"


struct thread_returns{
	Model * htn;
	int depth;
	bool block_compression;
	bool sat_mutexes;
	sat_pruning pruningMode;
	bool effectLessActionsInSeparateLeaf;
	PDT * pdt;
	//graph * dg;
	void* solver;
	int state;
	
	bool done;

// threading	
	int signal;
	pthread_t tid;
};
const int signalBase = 40;

bool current_done;

void* run_sat_planner_for_depth(void * param){
	thread_returns * ret = (thread_returns*) param;
	cout << THREAD_PREFIX << "Starting Thread for depth " << ret->depth << " @ signal " << ret->signal << endl;
	
	// set this thread to handle the 
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, signalBase + ret->signal);  
	pthread_sigmask(SIG_UNBLOCK, &sigmask, (sigset_t *)0);
	cout << THREAD_PREFIX << "I am handled by " << signalBase + ret->signal << endl;


	ret->pdt = new PDT(ret->htn);
	//ret->dg = compute_disabling_graph(ret->htn, true);
	ret->solver = ipasir_init();

	sat_capsule capsule;
	cout << "Generating formula for depth " << ret->depth << endl;
	MatchingData matching;
	createFormulaForDepth(ret->solver,ret->pdt,ret->htn,capsule,matching,ret->depth,ret->block_compression,ret->sat_mutexes, ret->pruningMode, ret->effectLessActionsInSeparateLeaf);
	cout << "Formula has " << capsule.number_of_variables << " vars and " << get_number_of_clauses() << " clauses." << endl;
	
	cout << "Starting solver" << endl;
	std::clock_t solver_start = std::clock();
	ret->state = ipasir_solve(ret->solver);
	std::clock_t solver_end = std::clock();
	double solver_time_in_ms = 1000.0 * (solver_end-solver_start) / CLOCKS_PER_SEC;
	cout << "Solver for depth " << ret->depth << " finished." << endl;
	cout << "Solver time: " << fixed << solver_time_in_ms << "ms" << endl;

	cout << "Solver state: " << (ret->state==10?"SAT":"UNSAT") << endl;
	if (ret->state == 10){
#ifndef NDEBUG
		printVariableTruth(ret->solver, ret->htn, capsule);
#endif
		printSolution(ret->solver, ret->htn, ret->pdt, matching);
		ipasir_release(ret->solver);
		exit(0);
	}
	// nothing to return;
	ret->done = true;
	current_done = true; // release the main thread
	return NULL;
}

pthread_mutex_t lock;

int signal_to_release = -1;


void handler(int dummy){
	cout << THREAD_PREFIX << "Suspension signal " << dummy << endl;
	while (true){
		std::this_thread::sleep_for(10ms);
		if (dummy == signal_to_release)
			break;
	}
	cout << THREAD_PREFIX << "Release signal " << dummy << endl;
}

template< class Rep, class Period >
void sleep_until_solver_finished(const std::chrono::duration<Rep, Period>& sleep_duration){
	for (int i = 0; i < 1000 / 25; i++){
		if (current_done) break;
		std::this_thread::sleep_for(25ms);
	}

	current_done = false;
}

void solve_with_sat_planner_time_interleave(Model * htn, bool block_compression, bool sat_mutexes, sat_pruning pruningMode, bool effectLessActionsInSeparateLeaf){
	int maxRuns = 6;
	current_done = false;
	vector<bool> takenSignals (maxRuns);

	sigset_t sigmask;
	struct sigaction action;
	/* Alle Bits auf null setzen */
	sigemptyset(&sigmask);
	/* Signal SIGUSR1 nicht blockieren ... */
	sigaddset(&sigmask, SIGINT);  
	pthread_sigmask(SIG_UNBLOCK, &sigmask, (sigset_t *)0);

   /* Setup Signal-Handler für SIGINT & SIGUSR1 */
	action.sa_flags = 0;
	action.sa_handler = handler;
	for (int i = 0; i < maxRuns; i++){
		cout << "Registering handler for " << signalBase + i << endl;
		sigaction(signalBase + i, &action, (struct sigaction *)0);
	}


	int depth = 1;
	vector<thread_returns*> runs;
	// iterate over time slices
	int positionOnRuns = -1;
	int runningSingal = 0;
	while (true){
		// stop whatever is currently running
		if (positionOnRuns != -1){
			signal_to_release = -1;
			cout << THREAD_PREFIX << "Stopping " << runs[positionOnRuns]->tid << " @ " << runningSingal << endl;
			std::this_thread::sleep_for(1ms);
   			pthread_kill(runs[positionOnRuns]->tid, signalBase + runningSingal);
			std::this_thread::sleep_for(10ms);
		}	

		cout << THREAD_PREFIX << "Switching to next task " << endl;
		do {
			positionOnRuns++;
		} while (runs.size() != positionOnRuns && runs[positionOnRuns]->done);
		cout << THREAD_PREFIX << "Next non-finished task is " << positionOnRuns << endl;

		if (runs.size() == positionOnRuns){ // last run, start a new one
			// check if we are eligible for starting a new run
			int activeRuns = 0;
			for (int i = 0; i < maxRuns; i++) takenSignals[i] = false;
			for (thread_returns * t : runs) if (!t->done){
				activeRuns++;
				takenSignals[t->signal] = true;
			}

			if (activeRuns < maxRuns){
				int firstFreeSignal = -1;
				for (int i = 0; i < maxRuns; i++)
					if (!takenSignals[i]){
						firstFreeSignal = i;
						break;
					}
		
				thread_returns* ret = new thread_returns();
				ret->htn = htn;
				ret->depth = depth++;
				ret->signal = firstFreeSignal;
				ret->done = false;
				ret->block_compression = block_compression;
				ret->sat_mutexes = sat_mutexes;
				ret->pruningMode = pruningMode;
				ret->effectLessActionsInSeparateLeaf = effectLessActionsInSeparateLeaf;
				runs.push_back(ret);
				
				//void *t1(void *);
				pthread_attr_t attr_obj; 
				pthread_attr_init(&attr_obj);
				pthread_create(&ret->tid, &attr_obj, run_sat_planner_for_depth, (void *)ret);
				cout << THREAD_PREFIX << "Starting worker: " << ret->tid << " @ " << firstFreeSignal << endl;
				
				// get this thread started
				sleep_until_solver_finished(1000ms);
				runningSingal = firstFreeSignal;
				continue;
				//printf("Haupt-Thread(%d) sendet SIGINT an TID(%d)\n", pthread_self(), ret->tid);
   				//pthread_kill(ret->tid, sig+firstFreeSignal);
				//printf("Haupt-Thread(%d) hat gesendet\n", pthread_self());
			} else {
				// it is not possible to start a new thread
				positionOnRuns = -1;
				do {
					positionOnRuns++;
				} while (runs.size() != positionOnRuns && runs[positionOnRuns]->done);
				cout << THREAD_PREFIX << "Not possible to start a new run, next non-finished task is " << positionOnRuns << endl;
			}
		}
		
		runningSingal = runs[positionOnRuns]->signal;
		cout << THREAD_PREFIX << "Letting " << positionOnRuns << " work @ " << runningSingal << endl;
		signal_to_release = signalBase + runningSingal;
		sleep_until_solver_finished(1000ms);
		cout << THREAD_PREFIX << positionOnRuns << " is done working" << endl;
	}
}



void solve_with_sat_planner(Model * htn, bool block_compression, bool sat_mutexes, sat_pruning pruningMode, bool effectLessActionsInSeparateLeaf, bool optimise){
	// prepare helper data structured (used for SOG)
	htn->calcSCCs();
	htn->constructSCCGraph();
	htn->analyseSCCcyclicity();
	
	optimisePlan = optimise;
	
	// start actual planner
	cout << endl << endl;
	// start by determining whether this model is totally ordered
	cout << "Instance is totally ordered: " << (htn->isTotallyOrdered?"yes":"no") << endl;
	//htn->writeToPDDL("foo-d.hddl", "foo-p.hddl");
	
	cout << color(Color::YELLOW,"Starting SAT-based planner") << endl;
	cout << "Using SAT solver: " << ipasir_signature() << endl;
	cout << "Encode Mutexes:    " << (sat_mutexes?"yes":"no") << endl;
	cout << "Block Compression: " << (block_compression?"yes":"no") << endl;
	cout << "Pruning:           ";
	switch (pruningMode){
		case SAT_NONE: cout << "none" << endl; break;
		case SAT_FF: cout << "ff" << endl; break;
		case SAT_H2: cout << "h2" << endl; break;
	}
	cout << "Optimise Plan Cost: " << (optimisePlan?"yes":"no") << endl;

	cout << endl << endl;
	
	// compute transitive closures of all methods
	htn->computeTransitiveClosureOfMethodOrderings();
	htn->buildOrderingDatastructures();

	
	originalActionCosts = new int[htn->numActions];
	for (int i = 0; i < htn->numActions; i++) originalActionCosts[i] = htn->actionCosts[i];

	if (useLogCostsForOptimisation){
		for (int i = 0; i < htn->numActions; i++)
			if (htn->actionCosts[i] != 0){
				htn->actionCosts[i] = 1 + log(htn->actionCosts[i]);
			}
	}


	//solve_with_sat_planner_time_interleave(htn, block_compression, sat_mutexes, pruningMode);
	solve_with_sat_planner_linear_bound_increase(htn, block_compression, sat_mutexes, pruningMode, effectLessActionsInSeparateLeaf);
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
