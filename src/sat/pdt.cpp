#include <cassert>
#include <fstream>
#include <iomanip>
#include <bitset>
#include "pdt.h"
#include "ipasir.h"
#include "../Util.h"

void printMemory();


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
	sog = nullptr;
}


PDT::PDT(Model* htn){
	path.clear();
	expanded = false;
	vertexVariables = false;
	childrenVariables = false;
	outputID = -1;
	outputTask = -1;
	outputMethod = -1;
	sog = nullptr;
	mother = nullptr;
	possibleAbstracts.push_back(htn->initialTask);
}

void PDT::resetPruning(Model * htn){
	prunedPrimitives.assign(possiblePrimitives.size(), false);
	prunedAbstracts.assign(possibleAbstracts.size(), false);
	
	if (expanded){
		prunedMethods.resize(possibleAbstracts.size());
		for (unsigned int at = 0; at < possibleAbstracts.size(); at++)
			prunedMethods[at].assign(htn->numMethodsForTask[possibleAbstracts[at]],false);
	}

	if (mother != nullptr){
		free(prunedCausesAbstractStart);
		free(prunedCausesForAbstract);
		prunedCausesAbstractStart = (uint32_t*) calloc(possibleAbstracts.size(),sizeof(uint32_t));
		int prunedCausesSize = 0;	
		for (unsigned int at = 0; at < possibleAbstracts.size(); at++){
			prunedCausesAbstractStart[at] = prunedCausesSize;
			prunedCausesSize += numberOfCausesPerAbstract[at];
		}
		prunedCausesSize = (prunedCausesSize + 63) / 64;
		prunedCausesForAbstract = (uint64_t*) calloc(prunedCausesSize, sizeof(uint64_t));
		for (int i = 0; i < prunedCausesSize; i++)
			prunedCausesForAbstract[i] = 0;
	
		free(prunedCausesPrimitiveStart);
		free(prunedCausesForPrimitive);
		prunedCausesPrimitiveStart = (uint32_t*) calloc(possiblePrimitives.size(),sizeof(uint32_t));
		prunedCausesSize = 0;	
		for (unsigned int p = 0; p < possiblePrimitives.size(); p++){
			prunedCausesPrimitiveStart[p] = prunedCausesSize;
			prunedCausesSize += numberOfCausesPerPrimitive[p];
		}
		prunedCausesSize = (prunedCausesSize + 63) / 64;
		prunedCausesForPrimitive = (uint64_t*) calloc(prunedCausesSize, sizeof(uint64_t));
		for (int i = 0; i < prunedCausesSize; i++)
			prunedCausesForPrimitive[i] = 0;
	}


	for (PDT * child : children)
		child->resetPruning(htn);
}

void PDT::initialisePruning(Model * htn){
	if (possiblePrimitives.size() != prunedPrimitives.size())
		prunedPrimitives.assign(possiblePrimitives.size(), false);
	if (possibleAbstracts.size() != prunedAbstracts.size())
		prunedAbstracts.assign(possibleAbstracts.size(), false);
	if (expanded && possibleAbstracts.size() != prunedMethods.size()){
		prunedMethods.resize(possibleAbstracts.size());
		for (unsigned int at = 0; at < possibleAbstracts.size(); at++)
			prunedMethods[at].assign(htn->numMethodsForTask[possibleAbstracts[at]],false);
	}
	if (mother != nullptr){
		if (prunedCausesForAbstract == 0){
			prunedCausesAbstractStart = (uint32_t*) calloc(possibleAbstracts.size(),sizeof(uint32_t));
			int prunedCausesSize = 0;	
			for (unsigned int at = 0; at < possibleAbstracts.size(); at++){
				prunedCausesAbstractStart[at] = prunedCausesSize;
				prunedCausesSize += numberOfCausesPerAbstract[at];
			}
			prunedCausesSize = (prunedCausesSize + 63) / 64;
			prunedCausesForAbstract = (uint64_t*) calloc(prunedCausesSize, sizeof(uint64_t));
			for (int i = 0; i < prunedCausesSize; i++)
				prunedCausesForAbstract[i] = 0;
		}
	
		if (prunedCausesForPrimitive == 0){	
			prunedCausesPrimitiveStart = (uint32_t*) calloc(possiblePrimitives.size(),sizeof(uint32_t));
			int prunedCausesSize = 0;	
			for (unsigned int p = 0; p < possiblePrimitives.size(); p++){
				prunedCausesPrimitiveStart[p] = prunedCausesSize;
				prunedCausesSize += numberOfCausesPerPrimitive[p];
			}
			prunedCausesSize = (prunedCausesSize + 63) / 64;
			prunedCausesForPrimitive = (uint64_t*) calloc(prunedCausesSize, sizeof(uint64_t));
			for (int i = 0; i < prunedCausesSize; i++)
				prunedCausesForPrimitive[i] = 0;
		}
	}
}

int SOGNUM = 0;

void PDT::expandPDT(Model* htn, bool effectLessActionsInSeparateLeaf){
	if (expanded) {
		//cout << "PDT is already expanded" << endl;
		return;
	}
	if (possibleAbstracts.size() == 0) {
		//cout << "Vertex has no assigned abstracts" << endl;
		return;
	}

#ifndef NDEBUG
	std::clock_t before_prep = std::clock();
	cout << endl << endl << "======================" << endl << "0 "; printMemory();
#endif
	expanded = true;

	
	vector<tuple<uint32_t,uint32_t,uint32_t>> applicableMethodsForSOG;
	// gather applicable methods
	for (int tIndex = 0; tIndex < possibleAbstracts.size(); tIndex++){
		int & t = possibleAbstracts[tIndex];
		for (int m = 0; m < htn->numMethodsForTask[t]; m++){
			applicableMethodsForSOG.push_back(make_tuple(htn->taskToMethods[t][m],tIndex,m));
		}
	}

	// get best possible SOG
#ifndef NDEBUG
	cout << "A "; printMemory();
	std::clock_t before_sog = std::clock();
#endif
	sog = optimiseSOG(applicableMethodsForSOG, htn, effectLessActionsInSeparateLeaf);
#ifndef NDEBUG
	std::clock_t after_sog = std::clock();
	cout << "Computed SOG, size: " << sog->numberOfVertices << endl;
	cout << "SOG "; printMemory();
#endif

	int c = 0;
	taskStartingPosition = (uint32_t*) calloc(possibleAbstracts.size(), sizeof(uint32_t));
	// gather applicable methods
	//listIndexOfChildrenForMethods.resize(possibleAbstracts.size());
	for (int tIndex = 0; tIndex < possibleAbstracts.size(); tIndex++){
		taskStartingPosition[tIndex] = c;
		int & t = possibleAbstracts[tIndex];
		//listIndexOfChildrenForMethods[tIndex].resize(htn->numMethodsForTask[t]);
		//for (int m = 0; m < htn->numMethodsForTask[t]; m++){
			//listIndexOfChildrenForMethods[tIndex][m].resize(sog->numberOfVertices);
			//for (size_t c = 0; c < sog->numberOfVertices; c++)
			//	listIndexOfChildrenForMethods[tIndex][m][c].present = false;
		//}
		c += htn->numMethodsForTask[t] * sog->numberOfVertices;
	}
	listIndexOfChildrenForMethods = (causePointer*) calloc(c,sizeof(causePointer));
	for (int i = 0; i < c; i++) listIndexOfChildrenForMethods[i].present = false;

#ifndef NDEBUG
	cout << "\t\t\tPA " << possibleAbstracts.size() << " " << c << endl;
	cout << "B "; printMemory();
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
	
#ifndef NDEBUG
	int listIndexOfChildrenForMethodsEntries = 0;
	int childPositions = 0;
	cout << "C "; printMemory();
#endif

	// create children
	for (size_t c = 0; c < sog->numberOfVertices; c++){
		PDT* child = new PDT(this);
		child->path = path;
		child->path.push_back(c);
		
		children.push_back(child);
	}
	
	vector<vector<vector<pair<int,int>>>> childCausesForAbstracts(sog->numberOfVertices);
	vector<vector<vector<pair<int,int>>>> childCausesForPrimitives(sog->numberOfVertices);
	
	for (size_t c = 0; c < sog->numberOfVertices; c++){
		PDT* child = children[c];

		for (const auto & [t,causes] : childrenTasks[c]){
			int subIndex; // index of the task in the child
			if (t < htn->numActions) {
				subIndex = child->possiblePrimitives.size();
				child->possiblePrimitives.push_back(t);
				childCausesForPrimitives[c].push_back(_empty);
				positionOfPrimitiveTasksInChildren [c][t] = subIndex;
#ifndef NDEBUG
		   		childPositions++;
#endif
			} else {
				subIndex = child->possibleAbstracts.size();
				child->possibleAbstracts.push_back(t);
				childCausesForAbstracts[c].push_back(_empty);
			}

			// go through the causes
			for (const auto & [tIndex,mIndex] : causes){
				int position = -1;
				if (t < htn->numActions){
					position = childCausesForPrimitives[c].back().size();
					childCausesForPrimitives[c].back().push_back(make_pair(tIndex,mIndex));
				} else {
					position = childCausesForAbstracts[c].back().size();
					childCausesForAbstracts[c].back().push_back(make_pair(tIndex,mIndex));
				}
				
				assert(!getListIndexOfChildrenForMethods(tIndex,mIndex,c)->present);
				
				getListIndexOfChildrenForMethods(tIndex,mIndex,c)->present = true;
				getListIndexOfChildrenForMethods(tIndex,mIndex,c)->isPrimitive = t < htn->numActions;
				getListIndexOfChildrenForMethods(tIndex,mIndex,c)->taskIndex = subIndex;
				getListIndexOfChildrenForMethods(tIndex,mIndex,c)->causeIndex = position;

#ifndef NDEBUG
				listIndexOfChildrenForMethodsEntries++;
#endif
			}
		}

	}
		

#ifndef NDEBUG
	cout << "D "; printMemory();
#endif

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
				if (effectLessActionsInSeparateLeaf){
					// childrenTasks[0][p] = _empty; no need to do this, as there are no duplicates in the list of primitives
					bool iameffectLess = htn->numAdds[p] == 0 && htn->numDels[p] == 0; 
					bool childSelected = false;
					for (size_t c = 0; c < sog->numberOfVertices; c++){
						if (childrenTasks[c].size() != 0){
							int itsFirst = childrenTasks[c].begin()->first;
							bool itEffectless = htn->numAdds[itsFirst] == 0 && htn->numDels[itsFirst] == 0;
							if (itEffectless != iameffectLess) continue;
						}
						// 
						selectedChild = c;
						int subIndex = children[selectedChild]->possiblePrimitives.size();
						children[selectedChild]->possiblePrimitives.push_back(p);
						childCausesForPrimitives[selectedChild].push_back(_empty);
						positionOfPrimitiveTasksInChildren[selectedChild][p] = subIndex;
						childSelected = true;
						break;
					}

					if (!childSelected){
						cout << "Edge case: no proper child selectable. Not yet implemented. mail g.behnke@uva.nl for bugfixing." << endl;
						exit(0);
					}
				} else {
					// just take 0
					// childrenTasks[0][p] = _empty; no need to do this, as there are no duplicates in the list of primitives
					selectedChild = 0;
					int subIndex = children[selectedChild]->possiblePrimitives.size();
					children[selectedChild]->possiblePrimitives.push_back(p);
					childCausesForPrimitives[selectedChild].push_back(_empty);
					positionOfPrimitiveTasksInChildren[selectedChild][p] = subIndex;	
				}
			}
			
			
			positionOfPrimitivesInChildren.push_back(make_tuple(
						selectedChild,
						positionOfPrimitiveTasksInChildren[selectedChild][p],
						childCausesForPrimitives[selectedChild][positionOfPrimitiveTasksInChildren[selectedChild][p]].size()
						));
			
			childCausesForPrimitives[selectedChild][positionOfPrimitiveTasksInChildren[selectedChild][p]].push_back(make_pair(-1,pIndex));
		}
	}


	for (size_t c = 0; c < sog->numberOfVertices; c++){
	
#ifndef NDEBUG
		if (effectLessActionsInSeparateLeaf){
			bool actualAction = false;
			bool effectLessAction = false;
			for (int prim : children[c]->possiblePrimitives){
				cout << htn->taskNames[prim];
				if (htn->numAdds[prim] > 0 || htn->numDels[prim] > 0){
					actualAction = true;
				} else {
					effectLessAction = true;
				}
			}

			if (actualAction && effectLessAction) {
				cout << "Non allowed mix of actions with effects and without." << endl;
				exit(0);
			}
		}
#endif



		// create causes data structures
		children[c]->numberOfCausesPerAbstract = (uint32_t*) calloc(children[c]->possibleAbstracts.size(),sizeof(uint32_t));
		children[c]->startOfCausesPerAbstract = (uint32_t*) calloc(children[c]->possibleAbstracts.size(),sizeof(uint32_t));
		int currentPosition = 0;
		for (int at = 0; at < children[c]->possibleAbstracts.size(); at++){
			children[c]->startOfCausesPerAbstract[at] = currentPosition;
			children[c]->numberOfCausesPerAbstract[at] = childCausesForAbstracts[c][at].size();
			currentPosition += children[c]->numberOfCausesPerAbstract[at];
		}

		children[c]->causesForAbstracts = (taskCause*) calloc(currentPosition, sizeof(taskCause));
		for (int at = 0; at < children[c]->possibleAbstracts.size(); at++){
			for (int cause = 0; cause < children[c]->numberOfCausesPerAbstract[at]; cause++){
				children[c]->getCauseForAbstract(at,cause)->taskIndex = childCausesForAbstracts[c][at][cause].first;
				children[c]->getCauseForAbstract(at,cause)->methodIndex = childCausesForAbstracts[c][at][cause].second;
			}
		}

		// and once for the primitives

		children[c]->numberOfCausesPerPrimitive = (uint32_t*) calloc(children[c]->possiblePrimitives.size(),sizeof(uint32_t));
		children[c]->startOfCausesPerPrimitive = (uint32_t*) calloc(children[c]->possiblePrimitives.size(),sizeof(uint32_t));
		currentPosition = 0;
		for (int at = 0; at < children[c]->possiblePrimitives.size(); at++){
			children[c]->startOfCausesPerPrimitive[at] = currentPosition;
			children[c]->numberOfCausesPerPrimitive[at] = childCausesForPrimitives[c][at].size();
			currentPosition += children[c]->numberOfCausesPerPrimitive[at];
		}

		children[c]->causesForPrimitives = (taskCause*) calloc(currentPosition, sizeof(taskCause));
		for (int at = 0; at < children[c]->possiblePrimitives.size(); at++){
			for (int cause = 0; cause < children[c]->numberOfCausesPerPrimitive[at]; cause++){
				children[c]->getCauseForPrimitive(at,cause)->taskIndex = childCausesForPrimitives[c][at][cause].first;
				children[c]->getCauseForPrimitive(at,cause)->methodIndex = childCausesForPrimitives[c][at][cause].second;
			}
		}
	}

#ifndef NDEBUG
	cout << "E "; printMemory();
	cout << "\t\tA" << setw(8) << possibleAbstracts.size() << " ";
	cout << "P" << setw(8) << possiblePrimitives.size() << " ";
	cout << "L" << setw(8) << listIndexOfChildrenForMethodsEntries << " ";
	cout << "P" << setw(8) << childPositions << endl;
	//cout << "\t\t\t\t" << listIndexOfChildrenForMethodsEntries * (sizeof(int)*3 + sizeof(bool)) / 1024 << endl;
#endif


#ifndef NDEBUG
	std::clock_t after_all = std::clock();
	double prep_in_ms = 1000.0 * (before_sog-before_prep) / CLOCKS_PER_SEC;
	double sog_in_ms = 1000.0 * (after_sog-before_sog) / CLOCKS_PER_SEC;
	double after_in_ms = 1000.0 * (after_all-after_sog) / CLOCKS_PER_SEC;
	cout << "SOG " << setw(8) << setprecision(3) << prep_in_ms << " " << setw(8) << setprecision(3) << sog_in_ms << " " << setw(8) << setprecision(3) << after_in_ms << " ms" << endl;
#endif

		
	if (htn->isTotallyOrdered)
		delete sog;

}


void PDT::expandPDTUpToLevel(int K, Model* htn, bool effectLessActionsInSeparateLeaf){
	assert(K >= 0);
	if (K == 0) return;
	
	expandPDT(htn, effectLessActionsInSeparateLeaf);

	for (PDT* child : children)
		child->expandPDTUpToLevel(K-1,htn, effectLessActionsInSeparateLeaf);
}

void PDT::getLeafs(vector<PDT*> & leafs){
	if (children.size()){
		for (PDT* child : children)
			child->getLeafs(leafs);
	} else
		leafs.push_back(this);
}

SOG* PDT::getLeafSOG(){
	if (children.size()){
		assert(sog != nullptr);
		vector<SOG*> sogs;
		for (PDT* child : children)
			sogs.push_back(child->getLeafSOG());
		return this->sog->expandSOG(sogs);
	} else {
		if (sog == nullptr)
			return generateSOGForLeaf(this);
		return sog;
	}
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
		if (cur->expanded){
			for (size_t m = 0; m < htn->numMethodsForTask[cur->possibleAbstracts[a]]; m++){
				printIndentMark(indent + 10, 10, cout);
				cout << color(cur->prunedMethods[a][m]?Color::RED : Color::WHITE, "m " + htn->methodNames[htn->taskToMethods[abs][m]]) << endl;
				//for (size_t s = 0; s < htn->numSubTasks[htn->taskToMethods[abs][m]]; s++){
				//	printIndentMark(indent + 15, 10, cout);
				//	cout << htn->taskNames[htn->subTasks[htn->taskToMethods[abs][m]][s]] << endl;
				//}
			}
		}
		/*if (cur->mother != nullptr){
			for (size_t c = 0; c < cur->causesForAbstracts[a].size(); c++){
				printIndentMark(indent + 10, 10, cout);
				cout << color(cur->prunedCausesForAbstract[a][c]?Color::RED : Color::WHITE, "c ") << endl;
			}
		}*/
	}

	for (PDT* child : cur->children)
		printPDT(htn,child,indent + 10);
}

void PDT::printDot(Model * htn, ofstream & dfile){
	if (! expanded) return;

	string s = "" + path_string_no_sep(path);
	/*
	for (int & a : possibleAbstracts)
		s += (s.size()?",":"") + htn->taskNames[a];
	for (int & p : possiblePrimitives)
		s += (s.size()?",":"") + htn->taskNames[p];
	*/
	dfile << "\ta" << path_string_no_sep(path) << "[label=\"" << s << "\"]" << endl;
	for (PDT* child : children){
		dfile << "\ta" << path_string_no_sep(path) << " -> a" << path_string_no_sep(child->path) << endl;
		child->printDot(htn,dfile);
	}
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

	if (!vertexVariables){
		primitiveVariable = (uint32_t*) calloc(possiblePrimitives.size(), sizeof(uint32_t));
		for (size_t p = 0; p < possiblePrimitives.size(); p++)
			primitiveVariable[p] = -1;
	}

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
	
	
	if (!vertexVariables){
		abstractVariable = (uint32_t*) calloc(possibleAbstracts.size(), sizeof(uint32_t));
		for (size_t a = 0; a < possibleAbstracts.size(); a++)
			abstractVariable[a] = -1;
	}

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

	methodVariablesStartIndex = (uint32_t*) calloc(possibleAbstracts.size(), sizeof(uint32_t));
	int methodVars = 0;	
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		methodVariablesStartIndex[a] = methodVars;
		methodVars += htn->numMethodsForTask[possibleAbstracts[a]];
	}
	methodVariables = (int32_t*) calloc(methodVars,sizeof(int32_t));
	for (int i = 0; i < methodVars; i++) methodVariables[i] = -1;

	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		for (size_t mi = 0; mi < htn->numMethodsForTask[possibleAbstracts[a]]; mi++){
#ifdef NO_PRUNED_VARIABLES
			if (prunedMethods[a][mi]) continue;
#endif
			// don't generate variables pertaining to children (and methods) twice
			if (*getMethodVariable(a,mi) != -1) continue;

			int num = capsule.new_variable();
			*getMethodVariable(a,mi) = num;

			DEBUG(
				int & t = possibleAbstracts[a];
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
		for (size_t pIndex = 0; pIndex < possiblePrimitives.size(); pIndex++){
			int prim = primitiveVariable[pIndex];
			if (prim == -1) continue; // pruned
			if (ipasir_val(solver,prim) > 0){
				assert(outputID == -1);
				outputID = currentID++;
				outputTask = possiblePrimitives[pIndex];
#ifndef NDEBUG
				cout << "Assigning " << outputID << " to atom " << prim << endl;
#endif
			}
		}
	
	for (size_t aIndex = 0; aIndex < possibleAbstracts.size(); aIndex++){
		int abs = abstractVariable[aIndex];
		if (abs == -1) continue; // pruned
		if (ipasir_val(solver,abs) > 0){
			assert(outputID == -1);
			outputID = currentID++;
			outputTask = possibleAbstracts[aIndex];
#ifndef NDEBUG
			cout << "Assigning " << outputID << " to atom " << abs << endl;
#endif

			// find the applied method
			for (size_t mIndex = 0; mIndex < htn->numMethodsForTask[possibleAbstracts[aIndex]]; mIndex++){
				int m = *getMethodVariable(aIndex,mIndex);
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

bool PDT::pruneCause(taskCause * cause){
	if (cause->taskIndex == -1){
		if (mother->prunedPrimitives[cause->methodIndex])
		   return false;	
		mother->prunedPrimitives[cause->methodIndex] = true;
	} else {
		if (mother->prunedMethods[cause->taskIndex][cause->methodIndex])
			return false;
		mother->prunedMethods[cause->taskIndex][cause->methodIndex] = true;
	}
	return true;
}

//#undef NDEBUG

void PDT::propagatePruning(Model * htn){
#ifndef NDEBUG
	cout << "Pruning at " << path_string(this->path) << endl; 
#endif
	// 1. Step: check what I can infer about me based on my surrounding

	// check whether I can prune an AT based on the methods
	// Methods were disabled by my children
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		if (prunedAbstracts[a]) continue; // at is already pruned

		bool all_methods_pruned = true;
		if (expanded)
			for (bool m : prunedMethods[a])
				all_methods_pruned &= m;

		if (all_methods_pruned){
#ifndef NDEBUG
			cout << "  A1 " << a << endl;
#endif
			prunedAbstracts[a] = true;
		}
	}

	if (mother != nullptr){
		// check whether it might be impossible for a primitive or abstract to be caused
		// This is where information flows from the top to the bottom	
		for (size_t p = 0; p < possiblePrimitives.size(); p++){
			if (prunedPrimitives[p]) continue;

			if (areAllCausesPrunedPrimitive(p)){
#ifndef NDEBUG
				cout << "  P  " << p << endl;
#endif
				prunedPrimitives[p] = true;
			}
		}


		for (size_t a = 0; a < possibleAbstracts.size(); a++){
			if (prunedAbstracts[a]) continue;

			if (areAllCausesPrunedAbstract(a)){
#ifndef NDEBUG
				cout << "  A2 " << a << endl;
#endif
				prunedAbstracts[a] = true;
			}
		}
	}


	for (size_t a = 0; a < possibleAbstracts.size(); a++)
		if (prunedAbstracts[a]) {
			if (expanded)
				for (size_t m = 0; m < htn->numMethodsForTask[possibleAbstracts[a]]; m++){
#ifndef NDEBUG
					cout << "  M  " << a << " " << m << endl;
#endif
					prunedMethods[a][m] = true; // if the AT is pruned, all methods are
				}
		}




	// my state has been determined
	///////////////////////////////

	// try to propagate information to my mother
	bool changedMother = false;

	if (mother != nullptr){
		for (size_t p = 0; p < possiblePrimitives.size(); p++)
			if (prunedPrimitives[p]){
				for (size_t c = 0; c < numberOfCausesPerPrimitive[p]; c++){
#ifndef NDEBUG
					cout << "  SS PC " << p << " " << c << endl;
#endif
					setPrunedCausesForPrimitive(p,c);
					changedMother |= pruneCause(getCauseForPrimitive(p,c));
				}
			}


		for (size_t a = 0; a < possibleAbstracts.size(); a++)
			if (prunedAbstracts[a]){
				for (size_t c = 0; c < numberOfCausesPerAbstract[a]; c++){
#ifndef NDEBUG
					cout << "  SS AC " << a << " " << c << endl;
#endif
					setPrunedCausesForAbstract(a,c);
					changedMother |= pruneCause(getCauseForAbstract(a,c));
				}
			}

	}

	// propagate towards children
	vector<bool> childrenChanged (children.size(), false);

	if (children.size()){
		for (size_t p = 0; p < possiblePrimitives.size(); p++)
			if (prunedPrimitives[p]){
				int childIndex = get<0>(positionOfPrimitivesInChildren[p]);
				int childPrimIndex = get<1>(positionOfPrimitivesInChildren[p]);
				int childCauseIndex = get<2>(positionOfPrimitivesInChildren[p]);
	
				if (!children[childIndex]->getPrunedCausesForPrimitive(childPrimIndex,childCauseIndex)){
#ifndef NDEBUG
					cout << "  SS CC " << childIndex << " " << childPrimIndex << " " << childCauseIndex << endl;
#endif
					childrenChanged[childIndex] = true;
					children[childIndex]->setPrunedCausesForPrimitive(childPrimIndex,childCauseIndex);
				}
			}
		
		for (size_t a = 0; a < possibleAbstracts.size(); a++)
			for (size_t m = 0; m < htn->numMethodsForTask[possibleAbstracts[a]]; m++){
				if (!prunedMethods[a][m]) continue;

				for (size_t childIndex = 0; childIndex < children.size(); childIndex++){
					if (!getListIndexOfChildrenForMethods(a,m,childIndex)->present) continue;
					bool isPrimitive = getListIndexOfChildrenForMethods(a,m,childIndex)->isPrimitive;
					int childTaskIndex = getListIndexOfChildrenForMethods(a,m,childIndex)->taskIndex;
					int childCauseIndex = getListIndexOfChildrenForMethods(a,m,childIndex)->causeIndex;
					bool causePruned = (isPrimitive) ? 
						children[childIndex]->getPrunedCausesForPrimitive(childTaskIndex,childCauseIndex):
						children[childIndex]->getPrunedCausesForAbstract(childTaskIndex,childCauseIndex);
					
					//cout << "  SS PCC " << childIndex << " " << childTaskIndex << " " << childCauseIndex << " " << causePruned <<" " << isPrimitive <<  endl;
					if (!causePruned){
						if (isPrimitive)  
							children[childIndex]->setPrunedCausesForPrimitive(childTaskIndex,childCauseIndex);
						else
							children[childIndex]->setPrunedCausesForAbstract(childTaskIndex,childCauseIndex);
						childrenChanged[childIndex] = true;
#ifndef NDEBUG	
						cout << "  SS PCC " << childIndex << " " << childTaskIndex << " " << childCauseIndex << endl;
#endif
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

//#define NDEBUG

void PDT::countPruning(int & overallSize, int & overallPruning, bool onlyPrimitives){
	for (size_t p = 0; p < possiblePrimitives.size(); p++)
		if (prunedPrimitives[p])
			overallPruning++;
	
	if (!onlyPrimitives)
		for (size_t a = 0; a < possibleAbstracts.size(); a++)
			if (prunedAbstracts[a])
				overallPruning++;

	overallSize += possiblePrimitives.size();
	if (!onlyPrimitives) overallSize += possibleAbstracts.size();
	
	for (PDT * child : children)
		child->countPruning(overallSize, overallPruning, onlyPrimitives);
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
			for (size_t m = 0; m < prunedMethods[a].size(); m++)
				if (prunedMethods[a][m] && *getMethodVariable(a,m) != -1)
					assertNot(solver, *getMethodVariable(a,m));
	}

	for (PDT * child : children)
		child->addPrunedClauses(solver);
			
}



void PDT::addDecompositionClauses(void* solver, sat_capsule & capsule, Model * htn){
	if (!expanded) return; // if I am not expanded, I have no decomposition clauses
	assert(vertexVariables);
	assert(childrenVariables);
#ifndef NDEBUG
	printMemory();
#endif
	// these clauses implement the rules of decomposition

	// at most one task
	vector<int> allTasks;
	for (int p = 0; p < possiblePrimitives.size(); p++)
		if (primitiveVariable[p] != -1)
			allTasks.push_back(primitiveVariable[p]);

	for (int a = 0; a < possibleAbstracts.size(); a++)
		if (abstractVariable[a] != -1)
			allTasks.push_back(abstractVariable[a]);
	
	if (allTasks.size() == 0) return; // no tasks can be here
#ifndef NDEBUG
	int stepA = get_number_of_clauses();
#endif
	atMostOne(solver,capsule, allTasks);	
#ifndef NDEBUG
	int stepB = get_number_of_clauses();
#endif

	// at most one method
	// TODO tested AMO per AT. This gives a lot more clauses ... (performance test is still open)
	vector<int> methodAtoms;
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		if (prunedAbstracts[a]) continue;
		for (size_t mi = 0; mi < htn->numMethodsForTask[possibleAbstracts[a]]; mi++){
			int i = *getMethodVariable(a,mi);
			if (i != -1)
				methodAtoms.push_back(i);
		}
	}
	if (methodAtoms.size())
		//assert(methodAtoms.size());
		atMostOne(solver,capsule, methodAtoms);
#ifndef NDEBUG
	int stepC = get_number_of_clauses();
#endif
	
	// if a primitive task is chosen, it must be inherited
	for (size_t p = 0; p < possiblePrimitives.size(); p++){
		if (primitiveVariable[p] == -1) continue; // pruned
		int child = get<0>(positionOfPrimitivesInChildren[p]);
		int pIndex = get<1>(positionOfPrimitivesInChildren[p]);
		implies(solver,primitiveVariable[p], children[child]->primitiveVariable[pIndex]);
		
		// TODO may be superflous as implied by causation of task in children
		for (size_t c = 0; c < children.size(); c++)
			if (child != c)
				implies(solver,primitiveVariable[p],children[c]->noTaskPresent);
	}
#ifndef NDEBUG
	int stepD = get_number_of_clauses();
#endif
	


	// if an abstract task is chosen

	// if a method is chosen, its abstract task must be true
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		uint32_t & av = abstractVariable[a];
		if (av == -1) continue; // pruned
		for (size_t mi = 0; mi < htn->numMethodsForTask[possibleAbstracts[a]]; mi++){
			int & mv = *getMethodVariable(a,mi);
			if (mv == -1) continue; // pruned
		
			// if the method is chosen, its abstract task has to be there
			implies(solver,mv,av);

			// this method will imply subtasks
			for (size_t child = 0; child < children.size(); child++){
				if (getListIndexOfChildrenForMethods(a,mi,child)->present) {
					bool isPrimitive = getListIndexOfChildrenForMethods(a,mi,child)->isPrimitive;
					int listIndex = getListIndexOfChildrenForMethods(a,mi,child)->taskIndex;
					
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
				} else {
					// TODO: try out the explicit encoding (m -> -task@child) for all children/tasks [this should lead to a larger encoding ..]
					implies(solver,mv,children[child]->noTaskPresent);
				}
			}
		}

		vector<int> temp;
		for (size_t mi = 0; mi < htn->numMethodsForTask[possibleAbstracts[a]]; mi++){
			int v = *getMethodVariable(a,mi);
			if (v != -1)
				temp.push_back(v);
		}
		assert(!expanded || temp.size());
		impliesOr(solver,av,temp);
	}
#ifndef NDEBUG
	int stepE = get_number_of_clauses();
#endif

	// selection of tasks at children must be caused by me
	// TODO: With AMO, this constraint should be implied, so we could leave it out
	for (PDT* & child : children){
		for (size_t a = 0; a < child->possibleAbstracts.size(); a++){
			if (child->abstractVariable[a] == -1) continue;

			vector<int> allCauses;
			for (size_t i = 0; i < child->numberOfCausesPerAbstract[a]; i++){
				if (child->getPrunedCausesForAbstract(a,i)) continue;
			
				taskCause * cause = child->getCauseForAbstract(a,i);
				int tIndex = cause->taskIndex;
				int mIndex = cause->methodIndex;

				assert(tIndex != -1);
				allCauses.push_back(*getMethodVariable(tIndex,mIndex));
			}

			impliesOr(solver, child->abstractVariable[a], allCauses);
		}

		for (size_t p = 0; p < child->possiblePrimitives.size(); p++){
			if (child->primitiveVariable[p] == -1) continue;
			
			vector<int> allCauses;
			
			for (size_t i = 0; i < child->numberOfCausesPerPrimitive[p]; i++){
				if (child->getPrunedCausesForPrimitive(p,i)) continue;


				taskCause * cause = child->getCauseForPrimitive(p,i);
				int tIndex = cause->taskIndex;
				int mIndex = cause->methodIndex;
				
				if (tIndex == -1){
					// is inherited
					allCauses.push_back(primitiveVariable[mIndex]);
				} else {
					allCauses.push_back(*getMethodVariable(tIndex,mIndex));
				}
			}

			impliesOr(solver, child->primitiveVariable[p], allCauses);
		}
	}
#ifndef NDEBUG
	int stepF = get_number_of_clauses();
#endif

	// if no task is present at this node, then none is actually here
	vector<int> temp;
	
	for (int p = 0; p < possiblePrimitives.size(); p++)
		if (primitiveVariable[p] != -1)
			temp.push_back(primitiveVariable[p]);
	if (temp.size())
		impliesAllNot(solver, noTaskPresent, temp);
#ifndef NDEBUG
	int stepG = get_number_of_clauses();
#endif


	temp.clear();
	for (int a = 0; a < possibleAbstracts.size(); a++)
		if (abstractVariable[a] != -1)
			temp.push_back(abstractVariable[a]);
	
	if (temp.size())
		impliesAllNot(solver, noTaskPresent, temp);
#ifndef NDEBUG
	int stepH = get_number_of_clauses();
#endif

	// no task present implies this for all children
	for (PDT* & child : children)
		implies(solver,noTaskPresent, child->noTaskPresent);
#ifndef NDEBUG
	int stepI = get_number_of_clauses();
#endif


#ifndef NDEBUG
	cout << "A: " << setw(8) << stepB - stepA << " ";
	cout << "B: " << setw(8) << stepC - stepB << " ";
	cout << "C: " << setw(8) << stepD - stepC << " ";
	cout << "D: " << setw(8) << stepE - stepD << " ";
	cout << "E: " << setw(8) << stepF - stepE << " ";
	cout << "F: " << setw(8) << stepG - stepF << " ";
	cout << "G: " << setw(8) << stepH - stepG << " ";
	cout << "I: " << setw(8) << stepI - stepH << endl;
#endif	
	
	// add clauses for children
	for (PDT* & child : children)
		child->addDecompositionClauses(solver, capsule, htn);
}


causePointer * PDT::getListIndexOfChildrenForMethods(int a, int b, int c){
	int base = taskStartingPosition[a];
	int add = base + b*children.size() + c;
	return listIndexOfChildrenForMethods + add;
}

bool PDT::getPrunedCausesForAbstract(int a, int b){
	int base = prunedCausesAbstractStart[a];
	int add = base + b;
	int num = add / 64;
	int bit = add % 64;

	return prunedCausesForAbstract[num] & (uint64_t(1) << bit);
}

void PDT::setPrunedCausesForAbstract(int a, int b){
	int base = prunedCausesAbstractStart[a];
	int add = base + b;
	int num = add / 64;
	int bit = add % 64;

	prunedCausesForAbstract[num] = prunedCausesForAbstract[num] | (uint64_t(1) << bit);
}


bool PDT::areAllCausesPrunedAbstract(int a){
	for (int c = 0; c < numberOfCausesPerAbstract[a]; c++)
		if (!getPrunedCausesForAbstract(a,c)) return false;
	return true;
}


bool PDT::getPrunedCausesForPrimitive(int a, int b){
	int base = prunedCausesPrimitiveStart[a];
	int add = base + b;
	int num = add / 64;
	int bit = add % 64;

	return prunedCausesForPrimitive[num] & (uint64_t(1) << bit);
}

void PDT::setPrunedCausesForPrimitive(int a, int b){
	int base = prunedCausesPrimitiveStart[a];
	int add = base + b;
	int num = add / 64;
	int bit = add % 64;

	prunedCausesForPrimitive[num] = prunedCausesForPrimitive[num] | (uint64_t(1) << bit);
}


bool PDT::areAllCausesPrunedPrimitive(int a){
	for (int c = 0; c < numberOfCausesPerPrimitive[a]; c++)
		if (!getPrunedCausesForPrimitive(a,c)) return false;
	return true;
}


int32_t * PDT::getMethodVariable(int a, int m){
	int base = methodVariablesStartIndex[a];
	int add = base + m;
	return methodVariables + add;
}

taskCause * PDT::getCauseForAbstract(int a, int i){
	int base = startOfCausesPerAbstract[a];
	int add = base + i;
	return causesForAbstracts + add;
}

taskCause * PDT::getCauseForPrimitive(int p, int i){
	int base = startOfCausesPerPrimitive[p];
	int add = base + i;
	return causesForPrimitives + add;
}

