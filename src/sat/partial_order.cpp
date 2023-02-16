#include "partial_order.h"


void generate_matching_formula(void* solver, sat_capsule & capsule, Model * htn, SOG* leafSOG, vector<vector<pair<int,int>>> & vars, MatchingData & matching){
	////////////////////////////////// matching variables
	matching.matchingPerLeaf.resize(leafSOG->numberOfVertices);
	matching.matchingPerPosition.resize(vars.size());
	matching.matchingPerPositionAMO.resize(vars.size());
	matching.vars = vars;
	matching.leafSOG = leafSOG;
	

	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		for (int p = 0; p < vars.size(); p++){
			int matchVar = capsule.new_variable();
			DEBUG(capsule.registerVariable(matchVar,"match leaf " + pad_int(l) + " + position " + pad_int(p)));
			matching.matchingPerLeaf[l].push_back(matchVar);
			matching.matchingPerPosition[p].push_back(matchVar);
			if (leafSOG->leafContainsEffectAction[l]){
				matching.matchingPerPositionAMO[p].push_back(matchVar);
			} else{
				DEBUG(cout << "Don't include " << l << "@" <<p << endl);
			}
		}	
	}


	vector<int> leafActive (leafSOG->numberOfVertices);
	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		int activeVar = capsule.new_variable();
		DEBUG(capsule.registerVariable(activeVar,"active leaf " + pad_int(l)));
		leafActive[l] = activeVar;
	}

	vector<int> positionActive (vars.size());
	for (int p = 0; p < vars.size(); p++){
		int activeVar = capsule.new_variable();
		DEBUG(capsule.registerVariable(activeVar,"active position " + pad_int(p)));
		positionActive[p] = activeVar;
	}

	////////////////////////////// constraints that
	
	// AMO one position per path
	for (int l = 0; l < leafSOG->numberOfVertices; l++)
		atMostOne(solver,capsule,matching.matchingPerLeaf[l]);
	
	// AMO paths per position 
	// but only consider paths that can actually contain an action with an effect
	for (int p = 0; p < vars.size(); p++)
		atMostOne(solver,capsule,matching.matchingPerPositionAMO[p]);


	// activity of leafs
	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		PDT * leaf = leafSOG->leafOfNode[l];
		vector<int> leafVariables;
		for (int prim = 0; prim < leaf->possiblePrimitives.size(); prim++){
			if (leaf->primitiveVariable[prim] == -1) continue; // pruned
			
			leafVariables.push_back(leaf->primitiveVariable[prim]);
		}

		impliesOr(solver,leafActive[l],leafVariables);
		notImpliesAllNot(solver,leafActive[l],leafVariables);
	}


	// if path is active one of its matchings must be true
	for (int l = 0; l < leafSOG->numberOfVertices; l++)
		impliesOr(solver,leafActive[l],matching.matchingPerLeaf[l]);

	// actions at positions must be caused
	for (int p = 0; p < vars.size(); p++){
		vector<int> positionVariables;
		for (auto [pvar,prim] : vars[p])
			if (htn->numAdds[prim] != 0 || htn->numDels[prim] != 0)
				positionVariables.push_back(pvar);

		impliesOr(solver,positionActive[p],positionVariables);
		notImpliesAllNot(solver,positionActive[p],positionVariables);
	}

	// if position is active one of its matchings must be true
	for (int p = 0; p < vars.size(); p++)
		impliesOr(solver,positionActive[p],matching.matchingPerPositionAMO[p]);



	for (int p = 0; p < vars.size(); p++){
		// variable data structure
		vector<int> variablesPerPrimitive(htn->numActions);

		for (auto & [pvar,prim] : vars[p])
			variablesPerPrimitive[prim] = pvar;
	
		// go through all leafs
		for (int l = 0; l < leafSOG->numberOfVertices; l++){
			PDT * leaf = leafSOG->leafOfNode[l];
			
			if (leafSOG->firstPossible[l] > p || leafSOG->lastPossible[l] < p){
				assertNot(solver,matching.matchingPerPosition[p][l]);
				continue;
			}

			for (int primC = 0; primC < leaf->possiblePrimitives.size(); primC++){
				if (leaf->primitiveVariable[primC] == -1) continue; // pruned
				int prim = leaf->possiblePrimitives[primC];

				// if this leaf is connected then the task must be present

				impliesAnd(solver,matching.matchingPerPosition[p][l],leaf->primitiveVariable[primC],variablesPerPrimitive[prim]);
			}
		}
	}
	
	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		if (!leafSOG->leafContainsEffectAction[l]) continue;
		// determine the possible primitives for this leaf
		vector<int> leafPrimitives (htn->numActions);
		PDT * leaf = leafSOG->leafOfNode[l];
		for (int primC = 0; primC < leaf->possiblePrimitives.size(); primC++){
			leafPrimitives[leaf->possiblePrimitives[primC]] = leaf->primitiveVariable[primC];
		}
		
		for (int p = 0; p < vars.size(); p++)
			for (auto [pvar,prim] : vars[p]){
				if (htn->numAdds[prim] != 0 || htn->numDels[prim] != 0){
				if (leafPrimitives[prim] > 0){
					// the leaf can potentially contain the action
						impliesAnd(solver,matching.matchingPerPosition[p][l],pvar,leafPrimitives[prim]);
				} else {
					// this is implicitly a bi-implication
					impliesNot(solver,matching.matchingPerPosition[p][l],pvar);
				}
				}
			}
	}

	////////////////////////// impose the encoded order
	vector<vector<int>> forbiddenPerLeaf (leafSOG->numberOfVertices);
	vector<vector<int>> forbiddenPerPosition (vars.size());

	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		for (int p = 0; p < vars.size(); p++){
			int forbiddenVar = capsule.new_variable();
			DEBUG(capsule.registerVariable(forbiddenVar,"forbidden leaf " + pad_int(l) + " + position " + pad_int(p)));
			forbiddenPerLeaf[l].push_back(forbiddenVar);
			forbiddenPerPosition[p].push_back(forbiddenVar);
		}	
	}


	// if leaf l @ position p, then any successor of l is forbidden at p-1
	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		for (int lSucc : leafSOG->adj[l]){
			for (int p = 1; p < vars.size(); p++){
				implies(solver,matching.matchingPerLeaf[l][p],forbiddenPerLeaf[lSucc][p-1]);
			}
		}
	}

	// forbidden-ness gets implied onwards
	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		for (int lSucc : leafSOG->adj[l]){
			for (int p = 0; p < vars.size(); p++){
				implies(solver, forbiddenPerLeaf[l][p], forbiddenPerLeaf[lSucc][p]);
			}
		}
	}

	// forbidden-ness gets implied on the positions
	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		for (int p = 1; p < vars.size(); p++){
			implies(solver, forbiddenPerLeaf[l][p], forbiddenPerLeaf[l][p-1]);
		}
	}


	// forbidden-ness forbids matchings
	for (int l = 0; l < leafSOG->numberOfVertices; l++){
		for (int p = 0; p < vars.size(); p++){
			impliesNot(solver,forbiddenPerLeaf[l][p],matching.matchingPerLeaf[l][p]);
		}
	}


	return;



	cout << "Hallo!" << endl;
}


