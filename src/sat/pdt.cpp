#include <cassert>
#include "pdt.h"
#include "../Util.h"


PDT* initialPDT(Model* htn){
	PDT* pdt = new PDT;
	pdt->expanded = false;
	pdt->possibleAbstracts.push_back(htn->initialTask);

	return pdt;
}


void expandPDT(PDT* pdt, Model* htn){
	if (pdt->expanded) {
		//cout << "PDT is already expanded" << endl;
		return;
	}
	if (pdt->possibleAbstracts.size() == 0) {
		//cout << "Vertex has no assigned abstracts" << endl;
		return;
	}

	vector<int> applicableMethodsForSOG;
	// gather applicable methods
	pdt->applicableMethods.resize(pdt->possibleAbstracts.size());
	for (int tIndex = 0; tIndex < pdt->possibleAbstracts.size(); tIndex++){
		int & t = pdt->possibleAbstracts[tIndex];
		for (int m = 0; m < htn->numMethodsForTask[t]; m++){
			pdt->applicableMethods[tIndex].push_back(true);
			applicableMethodsForSOG.push_back(htn->taskToMethods[t][m]);
		}
	}

	// get best possible SOG
	SOG * sog = optimiseSOG(applicableMethodsForSOG, htn);
#ifndef NDEBUG
	//cout << "Computed SOG, size: " << sog->numberOfVertices << endl;
#endif


	vector<unordered_set<int>> childrenTasks (sog->numberOfVertices);

	// assign tasks to children
	for (size_t mID = 0; mID < applicableMethodsForSOG.size(); mID++){
		int m = applicableMethodsForSOG[mID];
		assert(sog->methodSubTasksToVertices[mID].size() == htn->numSubTasks[m]);
		for (size_t sub = 0; sub < htn->numSubTasks[m]; sub++){
			int v = sog->methodSubTasksToVertices[mID][sub];
			int task = htn->subTasks[m][sub];
			childrenTasks[v].insert(task);
		}
	} 
	
	
	// create children
	for (size_t c = 0; c < sog->numberOfVertices; c++){
		pdt->children.push_back(new PDT());
		
		for (const int & t : childrenTasks[c])
			if (t < htn->numActions)
				pdt->children[c]->possiblePrimitives.push_back(t);
			else 
				pdt->children[c]->possibleAbstracts.push_back(t);
	}
}


void expandPDTUpToLevel(PDT* pdt, int K, Model* htn){
	assert(K >= 0);
	if (K == 0) return;
	
	expandPDT(pdt,htn);

	for (PDT* child : pdt->children)
		expandPDTUpToLevel(child,K-1,htn);
}

void getLeafs(PDT* cur, vector<PDT*> & leafs){
	if (cur->children.size()){
		for (PDT* child : cur->children)
			getLeafs(child, leafs);
	} else
		leafs.push_back(cur);
}


void printPDT(Model * htn, PDT* cur, int indent){
	printIndentMark(indent,10,cout); cout << "Node:" << endl;
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

