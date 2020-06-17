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

	// for every abstract, the task and method index by which it can be created
	// the task ID is relative to possibleAbstracts
	// the method ID relative to htn->numMethodsForTask for the actual task 
	vector<vector<pair<int,int>>> causesForAbstracts;
	// primitives can also be caused by primitive inheritance, this will be marked with a -1,primIndex (of parent)
	vector<vector<pair<int,int>>> causesForPrimitives;

	bool expanded;

	PDT* mother;
	vector<PDT*> children;
	SOG* sog;

	vector<vector<bool>> applicableMethods; // indicates whether the methods in the model are actually applicable and have not been pruned
	
	// Tuple
	// 1. Number of the child
	// 2. Primitive task assigned to child
	// 3. Index of the task in the child
	vector<vector<vector<tuple<int,bool,int>>>> listIndexOfChildrenForMethods;

	// for every primitive the number of the child and the index of the task in the possible primitives array
	vector<pair<int,int>> positionOfPrimitivesInChildren;
	
/// SAT encoding stuff
	bool vertexVariables;
	bool childrenVariables;
	int noTaskPresent;
	vector<int> primitiveVariable;
	vector<int> abstractVariable;
	vector<vector<int>> methodVariables;

///// that's what Tree Rex does ... for incremental solving
	int baseStateVarVariable;

/////  
	int outputID;
	int outputTask;
	int outputMethod;

///// METHODS
	PDT(PDT * m);
	PDT(Model* htn);


	void expandPDT(Model* htn);
	void getLeafs(vector<PDT*> & leafs);
	void expandPDTUpToLevel(int K, Model* htn);

	/**
	 * Recursively assigns variable IDs needed for encoding this PDT
	 */
	void assignVariableIDs(sat_capsule & capsule, Model * htn);
	
	void addDecompositionClauses(void* solver, sat_capsule & capsule);

	// output
	void assignOutputNumbers(void* solver, int & currentID, Model * htn);
	int getNextOutputTask();
	void printDecomposition(Model * htn);
};



void printPDT(Model * htn, PDT* cur);


#endif
