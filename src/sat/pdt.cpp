#include <cassert>
#include "pdt.h"
#include "ipasir.h"
#include "../Util.h"



//////////////////////////// actual PDT

PDT::PDT(){
	path.clear();
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
	possibleAbstracts.push_back(htn->initialTask);
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
		PDT* child = new PDT();
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
				listIndexOfChildrenForMethods[tIndex][mIndex].push_back(make_tuple(c,t < htn->numActions, subIndex));

				if (t < htn->numActions)
					child->causesForPrimitives.back().push_back(make_pair(tIndex,mIndex));
				else	
					child->causesForAbstracts.back().push_back(make_pair(tIndex,mIndex));
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
			
			positionOfPrimitivesInChildren.push_back(make_pair(selectedChild,positionOfPrimitiveTasksInChildren[selectedChild][p]));

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
	for (int prim : cur->possiblePrimitives){
		printIndentMark(indent + 5, 10, cout);
		cout << "p " << htn->taskNames[prim] << endl;
	}
	for (int abs : cur->possibleAbstracts){
		printIndentMark(indent + 5, 10, cout);
		cout << "a " << htn->taskNames[abs] << endl;
	}

	for (PDT* child : cur->children)
		printPDT(htn,child,indent + 10);
}


void printPDT(Model * htn, PDT* cur){
	printPDT(htn, cur,0);
}



void PDT::assignVariableIDs(sat_capsule & capsule, Model * htn){

	// don't generate vertex variables twice
	if (!vertexVariables){
		// variables are now generated
		vertexVariables = true;

		// a variable representing that no task at all is present at this vertex
		noTaskPresent = capsule.new_variable();
		DEBUG(
			string name = "none          @ " + pad_path(path);
			capsule.registerVariable(noTaskPresent,name);
		);	

		for (size_t p = 0; p < possiblePrimitives.size(); p++){
			int num = capsule.new_variable();
			primitiveVariable.push_back(num);

			DEBUG(
				string name = "prim     " + pad_int(possiblePrimitives[p]) + " @ " + pad_path(path) + ": " + pad_string(htn->taskNames[possiblePrimitives[p]]);
				capsule.registerVariable(num,name);
			);
		}

		for (size_t a = 0; a < possibleAbstracts.size(); a++){
			int num = capsule.new_variable();
			abstractVariable.push_back(num);

			DEBUG(
				string name = "abstract " + pad_int(possibleAbstracts[a]) + " @ " + pad_path(path) + ": " + pad_string(htn->taskNames[possibleAbstracts[a]]);
				capsule.registerVariable(num,name);
			);
		}
	}
	
	if (!expanded) return; // if I am not expanded, I have no decomposition clauses


	// don't generate variables pertaining to children (and methods) twice
	if (!childrenVariables){
		childrenVariables = true;
		
		methodVariables.resize(possibleAbstracts.size());
		for (size_t a = 0; a < possibleAbstracts.size(); a++){
			int & t = possibleAbstracts[a];
			for (size_t mi = 0; mi < applicableMethods[a].size(); mi++){
				int num = capsule.new_variable();
				methodVariables[a].push_back(num);

				DEBUG(
					int m = htn->taskToMethods[t][mi];
					string name = "method   " + pad_int(m) + " @ " + pad_path(path) + ": " + pad_string(htn->methodNames[m]);
					capsule.registerVariable(num,name);
				);
			}
		}
	}


	// generate variables for children
	for (PDT* & child : children)
		child->assignVariableIDs(capsule, htn);
}



void PDT::assignOutputNumbers(void* solver, int & currentID, Model * htn){
	if (outputID != -1) return;

	if (children.size() == 0)
		for (size_t pIndex = 0; pIndex < primitiveVariable.size(); pIndex++){
			int prim = primitiveVariable[pIndex];
			if (ipasir_val(solver,prim) > 0){
				assert(outputID == -1);
				outputID = currentID++;
				outputTask = possiblePrimitives[pIndex];
			}
		}
	
	for (size_t aIndex = 0; aIndex < abstractVariable.size(); aIndex++){
		int abs = abstractVariable[aIndex];
		if (ipasir_val(solver,abs) > 0){
			assert(outputID == -1);
			outputID = currentID++;
			outputTask = possibleAbstracts[aIndex];

			// find the applied method
			for (size_t mIndex = 0; mIndex < applicableMethods[aIndex].size(); mIndex++){
				int m = methodVariables[aIndex][mIndex];
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



void PDT::addDecompositionClauses(void* solver, sat_capsule & capsule){
	if (!expanded) return; // if I am not expanded, I have no decomposition clauses
	assert(vertexVariables);
	assert(childrenVariables);
	
	// these clauses implement the rules of decomposition

	// at most one task
	vector<int> allTasks;
	for (const int & x : primitiveVariable) allTasks.push_back(x);
	for (const int & x : abstractVariable)  allTasks.push_back(x);
	atMostOne(solver,capsule, allTasks);	

	// at most one method
	// TODO this is on an per AT basis, before it was one AMO for all methods
	for (size_t a = 0; a < possibleAbstracts.size(); a++)
		atMostOne(solver,capsule, methodVariables[a]);
	
	// if a primitive task is chosen, it must be inherited
	for (size_t p = 0; p < possiblePrimitives.size(); p++){
		//auto & [child,pIndex] = positionOfPrimitivesInChildren[p];
		int child = positionOfPrimitivesInChildren[p].first;
		int pIndex = positionOfPrimitivesInChildren[p].second;
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
		for (size_t mi = 0; mi < applicableMethods[a].size(); mi++){
			int & mv = methodVariables[a][mi];
		
			// if the method is chosen, its abstract task has to be there
			implies(solver,mv,av);

			//determine unassigned children for this method
			vector<bool> childAssigned;
			childAssigned.resize(children.size(),false);
			// this method will imply subtasks
			for (const auto & [child, isPrimitive, listIndex] : listIndexOfChildrenForMethods[a][mi]){
				childAssigned[child] = true;
				
				int childVariable;
				if (isPrimitive)
					childVariable = children[child]->primitiveVariable[listIndex];
				else
					childVariable = children[child]->abstractVariable[listIndex];

				implies(solver,mv,childVariable);
			}

			// TODO: try out the explicit encoding (m -> -task@child) for all children/tasks [this should lead to a larger encoding ..]
			for (size_t child = 0; child < children.size(); child++)
				if (!childAssigned[child])
					implies(solver,mv,children[child]->noTaskPresent);
		}

		impliesOr(solver,av,methodVariables[a]);
	}

	// selection of tasks at children must be caused by me
	// TODO: With AMO, this constraint should be implied, so we could leave it out
	for (PDT* & child : children){
		for (size_t a = 0; a < child->possibleAbstracts.size(); a++){
			vector<int> allCauses;
			for (const auto & [tIndex,mIndex] : child->causesForAbstracts[a]){
				assert(tIndex != -1);
				allCauses.push_back(methodVariables[tIndex][mIndex]);
			}

			impliesOr(solver, child->abstractVariable[a], allCauses);
		}

		for (size_t p = 0; p < child->possiblePrimitives.size(); p++){
			vector<int> allCauses;
			for (const auto & [tIndex,mIndex] : child->causesForPrimitives[p]){
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
	impliesAllNot(solver, noTaskPresent, primitiveVariable);
	impliesAllNot(solver, noTaskPresent, abstractVariable);

	// no task present implies this for all children
	for (PDT* & child : children)
		implies(solver,noTaskPresent, child->noTaskPresent);
		
	
	// add clauses for children
	for (PDT* & child : children)
		child->addDecompositionClauses(solver, capsule);
}
