/*
 * hhDOfree.h
 *
 *  Created on: 27.09.2018
 *      Author: Daniel Höller
 */

#ifndef HEURISTICS_HHDOFREE_H_
#define HEURISTICS_HHDOFREE_H_

#include <math.h>
#include <sys/time.h>
#include "../../Model.h"
#include "../../ProgressionNetwork.h"
#include "../../intDataStructures/noDelIntSet.h"
#include <iostream>
#include <vector>
#include <string>
#include <limits.h>
#include <set>
#include <ilcplex/ilocplex.h>
#include "../planningGraph.h"
#include "../../intDataStructures/IntUtil.h"

// how to create the model? default -> recreate
#ifndef DOFMODE
#define DOFMODE DOFRECREATE
#endif

namespace progression {

class hhDOfree {
	Model* htn;
	//void countTNI(searchNode* n, Model* htn);
	IntUtil iu;

	int* iUF;
	int* iUA;
	int* iM;
	int* iE;
	int* iTA;
	int* iTP;
	int* EStartI;
	int* EInvSize;
	int** iEInvEffIndex;
	int** iEInvActionIndex;

	int recreateModel(searchNode *n);

	planningGraph* pg;
	noDelIntSet preProReachable;
	void updatePG(searchNode *n);

#ifdef DOFADDLMCUTLMS

#endif


#ifdef DOFINREMENTAL

	bool* ILPcurrentFactReachability;
	IloExtractable* factReachability;

	bool* ILPcurrentTaskReachability;
	IloExtractable* taskReachability;

	// variable indices like above
	int* iS0;
	int* iTNI;

	// cplex stuff
	IloEnv lenv;
	IloModel model;
	IloNumVarArray v;
	IloCplex cplex;

	// formulae setting the values
	IloExtractable* initialState;
	IloExtractable* setStateBits;
	IloExtractable* initialTasks;

	// how it is set in the current model
	int* ILPcurrentS0;
	int* ILPcurrentTNI;

	void updateS0(searchNode *n);
	void updateTNI(searchNode *n);
	void updateRechability(searchNode *n);

#endif

#ifdef INITSCCS
	// mappings for the scc reachability, these are static
	int** sccNumIncommingMethods;
	int** sccNumInnerFromTo;
    int** sccNumInnerToFrom;
    int*** sccNumInnerFromToMethods;

    int*** sccIncommingMethods; // methods from another scc reaching a certain scc
    int*** sccInnerFromTo; // maps a tasks to those that it may be decomposed into
    int*** sccInnerToFrom; // maps a tasks to those that may be decomposed into it
    int**** sccInnerFromToMethods;

    // data structures for the indices
    int* RlayerPrev;
    int* RlayerCurrent;
    int** Ivars;
#endif

public:
	hhDOfree(Model* htn, searchNode *n);
	virtual ~hhDOfree();

	void setHeuristicValue(searchNode *n, searchNode *parent, int action);
	void setHeuristicValue(searchNode *n, searchNode *parent, int absTask,
			int method);
#ifdef DOFLMS
	void findLMs(searchNode *n);
	void ilpLMs(searchNode *n,
			int type,
			int id,
			int value,
			set<int>* tLMcandidates,
			set<int>* tLMs,
			set<int>* tUNRcandidates,
			set<int>* tUNRs,
			set<int>* mLMcandidates,
			set<int>* mLMs,
			set<int>* mUNRcandidates,
			set<int>* mUNRs,
			set<int>* fLMcandidates,
			set<int>* fLMs,
			set<int>* fUNRcandidates,
			set<int>* fUNRs
			);
#endif
private:
	void printInfo();
};

} /* namespace progression */

#endif /* HEURISTICS_HHDOFREE_H_ */

