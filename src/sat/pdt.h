#ifndef pdt_h_INCLUDED
#define pdt_h_INCLUDED

#include <stdint.h>
#include "../flags.h" // defines flags
#include "../Model.h"
#include "sog.h"
#include "sat_encoder.h"

struct causePointer {
	// 1. Primitive task assigned to child
	// 2. Index of the task in the child
	// 3. Index of the cause in the child
	bool present:1, isPrimitive:1;
	unsigned taskIndex : 31, causeIndex : 31;
};

struct taskCause {
	signed taskIndex : 32, methodIndex : 32;
};

struct PDT {
	vector<int> path;
	
	vector<int> possiblePrimitives;
	vector<int> possibleAbstracts;

	vector<bool> prunedPrimitives;
	vector<bool> prunedAbstracts;


	// for every abstract, the task and method index by which it can be created
	// the task ID is relative to possibleAbstracts
	// the method ID relative to htn->numMethodsForTask for the actual task 
	
	uint32_t * numberOfCausesPerAbstract;
	uint32_t * startOfCausesPerAbstract;
	taskCause * getCauseForAbstract(int a, int i);
	taskCause * causesForAbstracts;
	// primitives can also be caused by primitive inheritance, this will be marked with a -1,primIndex (of parent)
	
	uint32_t * numberOfCausesPerPrimitive;
	uint32_t * startOfCausesPerPrimitive;
	taskCause * getCauseForPrimitive(int p, int i);
	taskCause * causesForPrimitives;

	/// pruning information

	uint32_t * prunedCausesAbstractStart = 0;
	bool getPrunedCausesForAbstract(int a, int b);
	void setPrunedCausesForAbstract(int a, int b);
	bool areAllCausesPrunedAbstract(int a);
	uint64_t * prunedCausesForAbstract = 0;

	uint32_t * prunedCausesPrimitiveStart = 0;
	bool getPrunedCausesForPrimitive(int a, int b);
	void setPrunedCausesForPrimitive(int a, int b);
	bool areAllCausesPrunedPrimitive(int a);
	uint64_t * prunedCausesForPrimitive = 0;

	bool expanded;

	PDT* mother;
	vector<PDT*> children;
	SOG* sog;

	
	vector<vector<bool>> prunedMethods;

	//vector<vector<vector<causePointer>>> listIndexOfChildrenForMethods;
	uint32_t * taskStartingPosition;
	causePointer * listIndexOfChildrenForMethods;
	
	causePointer * getListIndexOfChildrenForMethods(int a, int b, int c);

	// for every primitive:
	// 1. Number of the child 
	// 2. Index of the task in the possible primitives array
	// 3. Index of cause in the child
	vector<tuple<uint16_t,uint32_t,uint32_t>> positionOfPrimitivesInChildren;
	
/// SAT encoding stuff
	bool vertexVariables;
	bool childrenVariables;
	int noTaskPresent;
	uint32_t * primitiveVariable;
	uint32_t * abstractVariable;


	uint32_t * methodVariablesStartIndex;
	int32_t * getMethodVariable(int a, int m);
	int32_t * methodVariables;

///// that's what Tree Rex does ... for incremental solving
	//int baseStateVarVariable;

/////  
	int outputID;
	int outputTask;
	int outputMethod;

///// METHODS
	PDT(PDT * m);
	PDT(Model* htn);

	void expandPDT(Model* htn, bool effectLessActionsInSeparateLeaf);
	void expandPDTUpToLevel(int K, Model* htn, bool effectLessActionsInSeparateLeaf);
	void getLeafs(vector<PDT*> & leafs);

	void initialisePruning(Model * htn);
	void resetPruning(Model * htn);
	void propagatePruning(Model * htn);

	void countPruning(int & overallSize, int & overallPruning, bool onlyPrimitives);

	/**
	 * Recursively assigns variable IDs needed for encoding this PDT
	 */
	void assignVariableIDs(sat_capsule & capsule, Model * htn);
	
	void addDecompositionClauses(void* solver, sat_capsule & capsule, Model * htn);

	void addPrunedClauses(void* solver);
	
	SOG* getLeafSOG();

	// output
	void assignOutputNumbers(void* solver, int & currentID, Model * htn);
	int getNextOutputTask();
	void printDecomposition(Model * htn);
	void printDot(Model * htn, ofstream & dfile);


private:
	bool pruneCause(taskCause * cause);
};



void printPDT(Model * htn, PDT* cur);


#endif
