#include <cassert>
#include "pdt.h"
#include "../Util.h"


string path_string(vector<int> & path){
	string s = "";
	for (int & i : path){
		if (s.size()) s+= ",";
		s+= to_string(i);
	}

	return s;
}

#define INTPAD 4
#define PATHPAD 15
#define STRINGPAD 0

string pad_string(string s, int chars = STRINGPAD){
	while (s.size() < chars)
		s += " ";
	return s;
}

string pad_int(int i, int chars = INTPAD){
	return pad_string(to_string(i),chars);
}

string pad_path(vector<int> & path, int chars = PATHPAD){
	return pad_string(path_string(path),chars);
}

//////////////////////////// actual PDT

PDT::PDT(){
	path.clear();
	expanded = false;
}


PDT::PDT(Model* htn){
	path.clear();
	expanded = false;
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
	
	
	// create children
	for (size_t c = 0; c < sog->numberOfVertices; c++){
		PDT* child = new PDT();
		child->path = path;
		child->path.push_back(c);

		children.push_back(child);
		
		for (const auto & [t,causes] : childrenTasks[c]){
			int subIndex; // index of the task in the child
			if (t < htn->numActions) {
				subIndex = children[c]->possiblePrimitives.size();
				children[c]->possiblePrimitives.push_back(t);
			} else {
				subIndex = children[c]->possibleAbstracts.size();
				children[c]->possibleAbstracts.push_back(t);
			}

			// go through the causes
			for (const auto & [tIndex,mIndex] : causes){
				listIndexOfChildrenForMethods[tIndex][mIndex].push_back(make_tuple(c,t < htn->numActions, subIndex));
			}
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

	for (size_t p = 0; p < possiblePrimitives.size(); p++){
		int num = capsule.new_variable();
		primitiveVariable.push_back(num);

		DEBUG(
			string name = "prim     " + pad_int(p) + " @ " + pad_path(path) + ": " + pad_string(htn->taskNames[possiblePrimitives[p]]);
			capsule.registerVariable(num,name);
		);
	}

	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		int num = capsule.new_variable();
		abstractVariable.push_back(num);

		DEBUG(
			string name = "abstract " + pad_int(a) + " @ " + pad_path(path) + ": " + pad_string(htn->taskNames[possibleAbstracts[a]]);
			capsule.registerVariable(num,name);
		);
	}
	
	if (!expanded) return; // if I am not expanded, I have no decomposition clauses
	
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


	// generate variables for children
	for (PDT* & child : children)
		child->assignVariableIDs(capsule, htn);
}


void PDT::addDecompositionClauses(void* solver){
	if (!expanded) return; // if I am not expanded, I have no decomposition clauses
	
	// these clauses implement the rules of decomposition


	// if an abstract task is chosen
	

	// if a method is chosen, its abstract task must be true
	for (size_t a = 0; a < possibleAbstracts.size(); a++){
		int & av = abstractVariable[a];
		for (size_t mi = 0; mi < applicableMethods[a].size(); mi++){
			int & mv = methodVariables[a][mi];
		
			implies(solver,mv,av);


			// this method will imply subtasks
			for (const auto & [child, isPrimitive, listIndex] : listIndexOfChildrenForMethods[a][mi]){
				int childVariable;
				if (isPrimitive)
					childVariable = children[child]->primitiveVariable[listIndex];
				else
					childVariable = children[child]->abstractVariable[listIndex];


				implies(solver,mv,childVariable);
			}
		}

		impliesOr(solver,av,methodVariables[a]);
	}	
	
	
	// add clauses for children
	for (PDT* & child : children)
		child->addDecompositionClauses(solver);
}
