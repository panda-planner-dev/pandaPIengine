#ifndef pdt_h_INCLUDED
#define pdt_h_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"
#include "sog.h"
#include "sat_encoder.h"


struct PDT {
	vector<int> path;
	
	vector<int> possiblePrimitives;
	vector<int> possibleAbstracts;


	bool expanded;

	int numberOfChildren;
	vector<PDT*> children;
	SOG* sog;

	vector<vector<bool>> applicableMethods; // indicates whether the methods in the model are actually applicable and have not been pruned
	vector<vector<vector<tuple<int,bool,int>>>> listIndexOfChildrenForMethods;
	vector<int> positionOfPrimitivesInChildren;
	
/// SAT encoding stuff
	vector<int> primitiveVariable;
	vector<int> abstractVariable;
	vector<vector<int>> methodVariables;

///// METHODS
	PDT();
	PDT(Model* htn);


	void expandPDT(Model* htn);
	void getLeafs(vector<PDT*> & leafs);
	void expandPDTUpToLevel(int K, Model* htn);

	/**
	 * Recursively assigns variable IDs needed for encoding this PDT
	 */
	void assignVariableIDs(sat_capsule & capsule, Model * htn);
	
	void addDecompositionClauses(void* solver);
};



void printPDT(Model * htn, PDT* cur);


#endif
