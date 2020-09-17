#include <cassert>
#include "pdt.h"
#include "ipasir.h"
#include "../Util.h"



//////////////////////////// actual PDT

PDT::PDT(PDT * m){
	path.clear();
	mother = m;
	expanded = false;
	vertexVariables = false;
	childrenVariables = false;
	outputID = -1;
	outputTask = -1;
	outputMethod = -1;
}


PDT::PDT(Model* htn){
	path.clear();
	expanded = false;
	vertexVariables = false;
	childrenVariables = false;
	outputID = -1;
	outputTask = -1;
	outputMethod = -1;
	mother = nullptr;
	possibleAbstracts.push_back(htn->initialTask);
}

void PDT::resetPruning(Model * htn){
	prunedPrimitives.assign(possiblePrimitives.size(), false);
	prunedAbstracts.assign(possibleAbstracts.size(), false);
	
	if (expanded){
		prunedMethods.resize(applicableMethods.size());
		for (unsigned int at = 0; at < possibleAbstracts.size(); at++)
			prunedMethods[at].assign(htn->numMethodsForTask[possibleAbstracts[at]],false);
	}

	if (mother != nullptr){
		prunedCausesForAbstract.resize(possibleAbstracts.size());
		for (unsigned int at = 0; at < possibleAbstracts.size(); at++)
			prunedCausesForAbstract[at].assign(causesForAbstracts[at].size(), false);
		prunedCausesForPrimitive.resize(possiblePrimitives.size());
		for (unsigned int p = 0; p < possiblePrimitives.size(); p++)
			prunedCausesForPrimitive[p].assign(causesForPrimitives[p].size(), false);
	}


	for (PDT * child : children)
		child->resetPruning(htn);
}

void PDT::initialisePruning(Model * htn){
	if (possiblePrimitives.size() != prunedPrimitives.size())
		prunedPrimitives.assign(possiblePrimitives.size(), false);
	if (possibleAbstracts.size() != prunedAbstracts.size())
		prunedAbstracts.assign(possibleAbstracts.size(), false);
	if (expanded && applicableMethods.size() != prunedMethods.size()){
		prunedMethods.resize(applicableMethods.size());
		for (unsigned int at = 0; at < possibleAbstracts.size(); at++)
			prunedMethods[at].assign(htn->numMethodsForTask[possibleAbstracts[at]],false);
	}
	if (mother != nullptr){
		if (prunedCausesForAbstract.size() != causesForAbstracts.size()){
			prunedCausesForAbstract.resize(possibleAbstracts.size());
			for (unsigned int at = 0; at < possibleAbstracts.size(); at++)
				prunedCausesForAbstract[at].assign(causesForAbstracts[at].size(), false);
		}
		if (prunedCausesForPrimitive.size() != causesForPrimitives.size()){
			prunedCausesForPrimitive.resize(possiblePrimitives.size());
			for (unsigned int p = 0; p < possiblePrimitives.size(); p++)
				prunedCausesForPrimitive[p].assign(causesForPrimitives[p].size(), false);
		}
	}
}

void PDT::expandPDT(Model* htn){
	if (expanded) {
		//cout << "PDT is already expanded" << endl;
		return;
	}
	if (possibleAbstracts.size() == 0) {
		//cout << "Vertex has no assigned abstracts" << endl;
		return;
	}

	expanded = true;

	vector<tuple<int,int,int>> applicableMethodsForSOG;
	// gather applicable methods
	applicableMethods.resize(possibleAbstracts.size());
	listIndexOfChildrenForMethods.resize(possibleAbstracts.size());
	for (int tIndex = 0; tIndex < possibleAbstracts.size(); tIndex++){
		int & t = possibleAbstracts[tIndex];
		listIndexOfChildrenForMethods[tIndex].resize(htn->numMethodsForTask[t]);
		for (int m = 0; m < htn->numMethodsForTask[t]; m++){
			applicableMethods[tIndex].push_back(true);
			applicableMethodsForSOG.push_back(make_tuple(htn->taskToMethods[t][m],tIndex,m));
		}
	}

	// get best possible SOG
	SOG * sog = optimiseSOG(applicableMethodsForSOG, htn);
#ifndef NDEBUG
	//cout << "Computed SOG, size: " << sog->numberOfVertices << endl;
#endif


	// maps tasks to possible causes (i.e. methods)
	vector<map<int,vector<pair<int,int>>>> childrenTasks (sog->numberOfVertices);

	// assign tasks to children
	for (size_t mID = 0; mID < applicableMethodsForSOG.size(); mID++){
		int m = get<0>(applicableMethodsForSOG[mID]);
		int tIndex = get<1>(applicableMethodsForSOG[mID]);
		int mIndex = get<2>(applicableMethodsForSOG[mID]);
		assert(sog->methodSubTasksToVertices[mID].size() == htn->numSubTasks[m]);
		for (size_t sub = 0; sub < htn->numSubTasks[m]; sub++){
			int v = sog->methodSubTasksToVertices[mID][sub];
			int task = htn->subTasks[m][sub];
			childrenTasks[v][task].push_back(make_pair(tIndex,mIndex));
		}
	} 
	
	vector<pair<int,int>> _empty;
	vector<map<int,int>> positionOfPrimitiveTasksInChildren (sog->numberOfVertices);
	
	// create children
	for (size_t c = 0; c < sog->numberOfVertices; c++){
		PDT* child = new PDT(this);
		child->path = path;
		child->path.push_back(c);

		for (const auto & [t,causes] : childrenTasks[c]){
			int subIndex; // index of the task in the child
			if (t < htn->numActions) {
				subIndex = child->possiblePrimitives.size();
				child->possiblePrimitives.push_back(t);
				child->causesForPrimitives.push_back(_empty);
				positionOfPrimitiveTasksInChildren [c][t] = subIndex;
		   	} else {
				subIndex = child->possibleAbstracts.size();
				child->possibleAbstracts.push_back(t);
				child->causesForAbstracts.push_back(_empty);
			}

			// go through the causes
			for (const auto & [tIndex,mIndex] : causes){
				
				int position = -1;
				if (t < htn->numActions){
					position = child->causesForPrimitives.back().size();
					child->causesForPrimitives.back().push_back(make_pair(tIndex,mIndex));
				} else {
					position = child->causesForAbstracts.back().size();
					child->causesForAbstracts.back().push_back(make_pair(tIndex,mIndex));
				}
				
				
				listIndexOfChildrenForMethods[tIndex][mIndex].push_back(make_tuple(
							c,
							t < htn->numActions,
						   	subIndex,
							position));

			}
		}
		
		
		children.push_back(child);
	}


	// determine inheritance for primitives
	if (sog->numberOfVertices > 0){ // if not we are a leaf and everything is ok
		for (size_t pIndex = 0; pIndex < possiblePrimitives.size(); pIndex++){
			int p = possiblePrimitives[pIndex];

			bool found = false;
			int selectedChild = -1;
			// look whether it is already there
			for (size_t c = 0; c < sog->numberOfVertices; c++)
				if (childrenTasks[c].count(p)){
					// found it!
					selectedChild = c;
					found = true;
					break;
				}

			if (!found) {
				// just take 0
				// childrenTasks[0][p] = _empty; no need to do this, as there are no duplicates in the list of primitives
				selectedChild = 0;
				int subIndex = children[selectedChild]->possiblePrimitives.size();
				children[selectedChild]->possiblePrimitives.push_back(p);
				children[selectedChild]->causesForPrimitives.push_back(_empty);
				positionOfPrimitiveTasksInChildren[selectedChild][p] = subIndex;
			}
			
			positionOfPrimitivesInChildren.push_back(make_tuple(
						selectedChild,
						positionOfPrimitiveTasksInChildren[selectedChild][p],
						children[selectedChild]->causesForPrimitives[positionOfPrimitiveTasksInChildren[selectedChild][p]].size()
						));

			children[selectedChild]->causesForPrimitives[positionOfPrimitiveTasksInChildren[selectedChild][p]].push_back(make_pair(-1,pIndex));
		}
	}
}


void PDT::expandPDTUpToLevel(int K, Model* htn){
	assert(K >= 0);
	if (K == 0) return;
	
	expandPDT(htn);

	for (PDT* child : children)
		child->expandPDTUpToLevel(K-1,htn);
}

void PDT::getLeafs(vector<PDT*> & leafs){
	if (children.size()){
		for (PDT* child : children)
			child->getLeafs(leafs);
	} else
		leafs.push_back(this);
}


void printPDT(Model * htn, PDT* cur, int indent){
	printIndentMark(indent,10,cout); cout << "Node: " << path_string(cur->path) << endl;
	for (size_t p = 0; p < cur->possiblePrimitives.size(); p++){
		int prim = cur->possiblePrimitives[p];
		printIndentMark(indent + 5, 10, cout);
		cout << color(cur->prunedPrimitives[p]?Color::RED : Color::WHITE, "p " + htn->taskNames[prim]) << endl;
	}
	for (size_t a = 0; a < cur->possibleAbstracts.size(); a++){
		int abs = cur->possibleAbstracts[a];
		printIndentMark(indent + 5, 10, cout);
		cout << color(cur->prunedAbstracts[a]?Color::RED : Color::WHITE, "a " + htn->taskNames[abs]) << endl;
	}

	for (PDT* child : cur->children)
		printPDT(htn,child,indent + 10);
}


void printPDT(Model * htn, PDT* cur){
	printPDT(htn, cur,0);
}


#define NO_PRUNED_VARIABLES


void PDT::assignVariableIDs(sat_capsule & capsule, Model * htn){

	// don't generate vertex variables twice
	if (!vertexVariables){

		// a variable representing that no task at all is present at this vertex
		noTaskPresent = capsule.new_variable();
		DEBUG(
			string name = "none          @ " + pad_path(path);
			capsule.registerVariable(noTaskPresent,name);
		);	
	}

	if (!vertexVariables)
		primitiveVariable.assign(possiblePrimitives.size(),-1);

	for (size_t p = 0; p < possiblePrimitives.size(); p++){
#ifdef NO_PRUNED_VARIABLES
		if (prunedPrimitives[p]) continue;
#endif
		// variable already assigned
		if (primitiveVariable[p] != -1) continue; // already generated

		int num = capsule.new_variable();
		primitiveVariable[p] = num;

		DEBUG(
			string name = "prim     " + pad_int(possiblePrimitives[p]) + " @ " + pad_path(path) + ": " + pad_string(htn->taskNames[possiblePrimitives[p]]);
			capsule.registerVariable(num,name);
		);
	}
	
	
	if (!vertexVariables)
		abstractVariable.assign(possibleAbstracts.size(),-1);

	for (size_t a = 0; a < possibleAbstracts.size(); a++){
#ifdef NO_PRUNED_VARIABLES
		if (prunedAbstracts[a]) continue;
#endif
		
		// variable already assigned
		if (abstractVariable[a] != -1) continue; // already generated
		
		int num = capsule.new_variable();
		abstractVariable[a] = num;

		DEBUG(
			string name = "abstract " + pad_int(possibleAbstracts[a]) + " @ " + pad_path(path) + ": " + pad_string(htn->taskNames[possibleAbstracts[a]]);
			capsule.registerVariable(num,name);
		);
	}
	
	
	// variables are now generated
	vertexVariables = true;


	if (!expanded) return; // if I am not expanded, I have no decomposition clauses

	methodVariables.resize(possibleAbstracts.size());
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		int & t = possibleAbstracts[a];
		methodVariables[a].assign(applicableMethods[a].size(),-1);
		for (size_t mi = 0; mi < applicableMethods[a].size(); mi++){
#ifdef NO_PRUNED_VARIABLES
			if (prunedMethods[a][mi]) continue;
#endif
			// don't generate variables pertaining to children (and methods) twice
			if (methodVariables[a][mi] != -1) continue;

			int num = capsule.new_variable();
			methodVariables[a][mi] = num;

			DEBUG(
				int m = htn->taskToMethods[t][mi];
				string name = "method   " + pad_int(m) + " @ " + pad_path(path) + ": " + pad_string(htn->methodNames[m]);
				capsule.registerVariable(num,name);
			);
		}
	}
	
	
	childrenVariables = true;


	// generate variables for children
	for (PDT* & child : children)
		child->assignVariableIDs(capsule, htn);
}



void PDT::assignOutputNumbers(void* solver, int & currentID, Model * htn){
	if (outputID != -1) return;

	if (children.size() == 0)
		for (size_t pIndex = 0; pIndex < primitiveVariable.size(); pIndex++){
			int prim = primitiveVariable[pIndex];
			if (prim == -1) continue; // pruned
			if (ipasir_val(solver,prim) > 0){
				assert(outputID == -1);
				outputID = currentID++;
				outputTask = possiblePrimitives[pIndex];
			}
		}
	
	for (size_t aIndex = 0; aIndex < abstractVariable.size(); aIndex++){
		int abs = abstractVariable[aIndex];
		if (abs == -1) continue; // pruned
		if (ipasir_val(solver,abs) > 0){
			assert(outputID == -1);
			outputID = currentID++;
			outputTask = possibleAbstracts[aIndex];

			// find the applied method
			for (size_t mIndex = 0; mIndex < applicableMethods[aIndex].size(); mIndex++){
				int m = methodVariables[aIndex][mIndex];
				if (m == -1) continue; // pruned
				if (ipasir_val(solver,m) > 0){
					assert(outputMethod == -1);
					outputMethod = htn->taskToMethods[outputTask][mIndex];
				}
			}
		}
	}

	for (PDT* & child : children) child->assignOutputNumbers(solver, currentID, htn);
}

int PDT::getNextOutputTask(){
	if (outputID != -1) return outputID;
	
	for (PDT* & child : children){
		int sub = child->getNextOutputTask();
		if (sub != -1) return sub;
	}
	return -1;
}

void PDT::printDecomposition(Model * htn){
	if (outputID == -1) return;
	if (outputTask < htn->numActions) return;
	
	cout << outputID << " " << htn->taskNames[outputTask] << " -> " << 
		htn->methodNames[outputMethod];

	// output children
	for (PDT* & child : children){
		int sub = child->getNextOutputTask();
		if (sub != -1) cout << " " << sub;
	}
	cout << endl;

	for (PDT* & child : children) child->printDecomposition(htn);
}

bool PDT::pruneCause(pair<int,int> & cause){
	if (cause.first == -1){
		if (mother->prunedPrimitives[cause.second])
		   return false;	
		mother->prunedPrimitives[cause.second] = true;
	} else {
		if (mother->prunedMethods[cause.first][cause.second])
			return false;
		mother->prunedMethods[cause.first][cause.second] = true;
	}
	return true;
}


void PDT::propagatePruning(Model * htn){


	// check whether I can prune an AT based on the methods
	// Methods were disabled by my children
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		if (prunedAbstracts[a]) continue; // at is already pruned

		bool all_methods_pruned = true;
		if (expanded)
			for (bool m : prunedMethods[a])
				all_methods_pruned &= m;

		if (all_methods_pruned)
			prunedAbstracts[a] = true;
	}

	if (mother != nullptr){
		// check whether it might be impossible for a primitive or abstract to be caused
		// This is where information flows from the top to the bottom	
		for (size_t p = 0; p < possiblePrimitives.size(); p++){
			if (prunedPrimitives[p]) continue;

			bool all_causes_pruned = true;
			for (bool c : prunedCausesForPrimitive[p])
				all_causes_pruned &= c;
			if (all_causes_pruned)
				prunedPrimitives[p] = true;
		}


		for (size_t a = 0; a < possibleAbstracts.size(); a++){
			if (prunedAbstracts[a]) continue;

			bool all_causes_pruned = true;
			for (bool c : prunedCausesForAbstract[a])
				all_causes_pruned &= c;
			if (all_causes_pruned)
				prunedAbstracts[a];
		}
	}


	for (size_t a = 0; a < possibleAbstracts.size(); a++)
		if (prunedAbstracts[a]) {
			if (expanded)
				for (size_t m = 0; m < applicableMethods[a].size(); m++)
					prunedMethods[a][m] = true; // if the AT is pruned, all methods are
		}




	// my state has been determined
	///////////////////////////////

	// try to propagate information to my mother
	bool changedMother = false;

	if (mother != nullptr){
		for (size_t p = 0; p < possiblePrimitives.size(); p++)
			if (prunedPrimitives[p])
				// any method
				for (pair<int,int> & cause : causesForPrimitives[p])
					changedMother |= pruneCause(cause);


		for (size_t a = 0; a < possibleAbstracts.size(); a++)
			if (prunedAbstracts[a])
				// any method
				for (pair<int,int> & cause : causesForAbstracts[a])
					changedMother |= pruneCause(cause);

	}

	// propagate towards children
	vector<bool> childrenChanged (children.size(), false);

	if (children.size()){
		for (size_t p = 0; p < possiblePrimitives.size(); p++)
			if (prunedPrimitives[p]){
				int childIndex = get<0>(positionOfPrimitivesInChildren[p]);
				int childPrimIndex = get<1>(positionOfPrimitivesInChildren[p]);
				int childCauseIndex = get<2>(positionOfPrimitivesInChildren[p]);
	
				if (!children[childIndex]->prunedCausesForPrimitive[childPrimIndex][childCauseIndex]){
					childrenChanged[childIndex] = true;
					children[childIndex]->prunedCausesForPrimitive[childPrimIndex][childCauseIndex] = true;
				}
			}
		
		for (size_t a = 0; a < possibleAbstracts.size(); a++)
			for (size_t m = 0; m < applicableMethods[a].size(); m++){
				if (!prunedMethods[a][m]) continue;

				for (auto [childIndex, isPrimitive, childTaskIndex, childCauseIndex] : listIndexOfChildrenForMethods[a][m]){
					bool causePruned = (isPrimitive) ? 
						children[childIndex]->prunedCausesForPrimitive[childTaskIndex][childCauseIndex]:
						children[childIndex]->prunedCausesForAbstract[childTaskIndex][childCauseIndex];
	
					if (!causePruned){
						if (isPrimitive)  
							children[childIndex]->prunedCausesForPrimitive[childTaskIndex][childCauseIndex] = true;
						else
							children[childIndex]->prunedCausesForAbstract[childTaskIndex][childCauseIndex] = true;
						childrenChanged[childIndex] = true;
					}
				}
			}
	}
	

	// propagate to children
	for (size_t childIndex = 0; childIndex < children.size(); childIndex++)
		if (childrenChanged[childIndex])
			children[childIndex]->propagatePruning(htn);
	
	if (changedMother)
	   mother->propagatePruning(htn);	
}

void PDT::countPruning(int & overallSize, int & overallPruning){
	for (size_t p = 0; p < possiblePrimitives.size(); p++)
		if (prunedPrimitives[p])
			overallPruning++;
	
	for (size_t a = 0; a < possibleAbstracts.size(); a++)
		if (prunedAbstracts[a])
			overallPruning++;

	overallSize += possiblePrimitives.size() + possibleAbstracts.size();
	for (PDT * child : children)
		child->countPruning(overallSize, overallPruning);
}

void PDT::addPrunedClauses(void* solver){
	for (size_t p = 0; p < possiblePrimitives.size(); p++)
		if (prunedPrimitives[p] && primitiveVariable[p] != -1)
			assertNot(solver,primitiveVariable[p]);
	
	for (size_t a = 0; a < possibleAbstracts.size(); a++)
		if (prunedAbstracts[a] && abstractVariable[a] != -1)
			assertNot(solver,abstractVariable[a]);
	
	if (expanded){
		for (size_t a = 0; a < possibleAbstracts.size(); a++)
			for (size_t m = 0; m < applicableMethods[a].size(); m++)
				if (prunedMethods[a][m] && methodVariables[a][m] != -1)
					assertNot(solver, methodVariables[a][m]);
	}

	for (PDT * child : children)
		child->addPrunedClauses(solver);
			
}


void PDT::addDecompositionClauses(void* solver, sat_capsule & capsule){
	if (!expanded) return; // if I am not expanded, I have no decomposition clauses
	assert(vertexVariables);
	assert(childrenVariables);
	
	// these clauses implement the rules of decomposition

	// at most one task
	vector<int> allTasks;
	for (const int & x : primitiveVariable) 
		if (x != -1)
			allTasks.push_back(x);
	for (const int & x : abstractVariable)
		if (x != -1)
			allTasks.push_back(x);
	
	if (allTasks.size() == 0) return; // no tasks can be here
	atMostOne(solver,capsule, allTasks);	

	// at most one method
	// TODO this is on an per AT basis, before it was one AMO for all methods
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		if (prunedAbstracts[a]) continue;
		vector<int> temp;
		for (const int & i : methodVariables[a])
			if (i != -1)
				temp.push_back(i);
		
		assert(temp.size());
		atMostOne(solver,capsule, temp);
	}
	
	// if a primitive task is chosen, it must be inherited
	for (size_t p = 0; p < possiblePrimitives.size(); p++){
		if (primitiveVariable[p] == -1) continue; // pruned
		//auto & [child,pIndex] = positionOfPrimitivesInChildren[p];
		int child = get<0>(positionOfPrimitivesInChildren[p]);
		int pIndex = get<1>(positionOfPrimitivesInChildren[p]);
		implies(solver,primitiveVariable[p], children[child]->primitiveVariable[pIndex]);
		
		// TODO may be superflous as implied by causation of task in children
		for (size_t c = 0; c < children.size(); c++)
			if (child != c)
				implies(solver,primitiveVariable[p],children[c]->noTaskPresent);
	}
	


	// if an abstract task is chosen

	// if a method is chosen, its abstract task must be true
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		int & av = abstractVariable[a];
		if (av == -1) continue; // pruned
		for (size_t mi = 0; mi < applicableMethods[a].size(); mi++){
			int & mv = methodVariables[a][mi];
			if (mv == -1) continue; // pruned
		
			// if the method is chosen, its abstract task has to be there
			implies(solver,mv,av);

			//determine unassigned children for this method
			vector<bool> childAssigned;
			childAssigned.resize(children.size(),false);
			// this method will imply subtasks
			for (const auto & [child, isPrimitive, listIndex,_] : listIndexOfChildrenForMethods[a][mi]){
				childAssigned[child] = true;
				
				int childVariable;
				if (isPrimitive)
					childVariable = children[child]->primitiveVariable[listIndex];
				else
					childVariable = children[child]->abstractVariable[listIndex];

				if (childVariable == -1){
					cout << "OO" << endl;
					exit(0);
				}

				implies(solver,mv,childVariable);
			}

			// TODO: try out the explicit encoding (m -> -task@child) for all children/tasks [this should lead to a larger encoding ..]
			for (size_t child = 0; child < children.size(); child++)
				if (!childAssigned[child])
					implies(solver,mv,children[child]->noTaskPresent);
		}

		vector<int> temp;
		for (const int & v : methodVariables[a])
			if (v != -1)
				temp.push_back(v);
		assert(!expanded || temp.size());
		impliesOr(solver,av,temp);
	}

	// selection of tasks at children must be caused by me
	// TODO: With AMO, this constraint should be implied, so we could leave it out
	for (PDT* & child : children){
		for (size_t a = 0; a < child->possibleAbstracts.size(); a++){
			if (child->abstractVariable[a] == -1) continue;

			vector<int> allCauses;
			for (size_t i = 0; i < child->causesForAbstracts[a].size(); i++){
				if (child->prunedCausesForAbstract[a][i]) continue;
			
				const auto & [tIndex,mIndex] = child->causesForAbstracts[a][i];
				assert(tIndex != -1);
				allCauses.push_back(methodVariables[tIndex][mIndex]);
			}

			impliesOr(solver, child->abstractVariable[a], allCauses);
		}

		for (size_t p = 0; p < child->possiblePrimitives.size(); p++){
			if (child->primitiveVariable[p] == -1) continue;
			
			vector<int> allCauses;
			
			for (size_t i = 0; i < child->causesForPrimitives[p].size(); i++){
				if (child->prunedCausesForPrimitive[p][i]) continue;


				const auto & [tIndex,mIndex] = child->causesForPrimitives[p][i];
				if (tIndex == -1){
					// is inherited
					allCauses.push_back(primitiveVariable[mIndex]);
				} else {
					allCauses.push_back(methodVariables[tIndex][mIndex]);
				}
			}

			impliesOr(solver, child->primitiveVariable[p], allCauses);
		}
	}

	// if no task is present at this node, then none is actually here
	vector<int> temp;
	for (const int & v : primitiveVariable)
		if (v != -1)
			temp.push_back(v);
	if (temp.size())
		impliesAllNot(solver, noTaskPresent, temp);


	temp.clear();
	for (const int & v : abstractVariable)
		if (v != -1)
			temp.push_back(v);
	if (temp.size())
		impliesAllNot(solver, noTaskPresent, temp);

	// no task present implies this for all children
	for (PDT* & child : children)
		implies(solver,noTaskPresent, child->noTaskPresent);
		
	
	// add clauses for children
	for (PDT* & child : children)
		child->addDecompositionClauses(solver, capsule);
}
