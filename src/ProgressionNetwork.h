/*
 * ProgressionNetwork.h
 *
 *  Created on: 07.09.2017
 *      Author: Daniel Höller
 */

#ifndef PROGRESSIONNETWORK_H_
#define PROGRESSIONNETWORK_H_

#include "flags.h"
#include <vector>
#include <unordered_set>
#include <functional>
#include <map>
#include <set>
#include <iostream>
#include <forward_list>
#include "heuristics/landmarks/lmDataStructures/landmark.h"
#include "heuristics/landmarks/lmDataStructures/lookUpTab.h"
#include "flags.h"
//#include "heuristics/HeuristicPayload.h"

using namespace std;

namespace progression {


// forward declaration due to cyclic dependency
struct Model;


#ifdef TRACESOLUTION
extern int currentSolutionStepInstanceNumber;
#endif
#ifdef SAVESEARCHSPACE
extern int currentSearchNodeID;
#endif 

struct solutionStep {
	int task;
	int method;
	int pointersToMe;
#ifdef TRACESOLUTION
	int mySolutionStepInstanceNumber;
	int myPositionInParent;
	int parentSolutionStepInstanceNumber;
#endif
	solutionStep* prev;

	~solutionStep();
};

struct planStep {
	int id;
	int task;
	int pointersToMe;
#ifdef TRACESOLUTION
	int myPositionInParent;
	int parentSolutionStepInstanceNumber;
#endif
	int numSuccessors;
	planStep** successorList = nullptr;
	int numReachableT;
	int* reachableT = nullptr;

    // todo: delete the following two values when RC is replaced by RC2
	int numGoalFacts;
	int* goalFacts = nullptr;

	bool operator==(const planStep &that) const;

	~planStep();
};

struct searchNode {
	vector<bool> state;
	int numAbstract;
	int numPrimitive;
	planStep** unconstraintAbstract;
	planStep** unconstraintPrimitive;

	int* heuristicValue = nullptr;
	int fValue;
	bool goalReachable = true;
	int modificationDepth;
	int mixedModificationDepth;
	int actionCosts = 0;

//    HeuristicPayload** hPL = nullptr;

	solutionStep* solution;

	bool operator<(searchNode other) const;

	~searchNode();
	searchNode();

	int hRand;

#ifdef SAVESEARCHSPACE
	int searchNodeID;
#endif 

	int numContainedTasks = 0;
	int* containedTasks = nullptr;
	int* containedTaskCount = nullptr;

#ifdef TRACKLMS
	// obsolete, will be removed use TRACKLMSFULL
	int numfLMs = 0; // number of fact landmarks *left*
	int numtLMs = 0; // number of task landmarks *left*
	int nummLMs = 0; // number of method landmarks *left*
	int* fLMs; // fact landmarks
	int* tLMs; // task landmarks
	int* mLMs; // method landmarks

	int reachedfLMs = 0; // number of fact landmarks already *reached*
	int reachedtLMs = 0; // number of task landmarks already *reached*
	int reachedmLMs = 0; // number of method landmarks already *reached*
#endif

#ifdef TRACKLMSFULL
	int numLMs = 0;
	landmark** lms;

	lookUpTab* lookForT = nullptr;
	lookUpTab* lookForF = nullptr;
	lookUpTab* lookForM = nullptr;

	int reachedfLMs = 0; // number of fact landmarks already *reached*
	int reachedtLMs = 0; // number of task landmarks already *reached*
	int reachedmLMs = 0; // number of method landmarks already *reached*
#endif


	void printNode(std::ostream & out);
	void node2Dot(std::ostream & out);

private:
	void printDFS(planStep * s, map<planStep*,int> & psp, set<pair<planStep*,planStep*>> & orderpairs);
};

///////////////////// Functions for extracting results from search nodes
pair<string,int> extractSolutionFromSearchNode(Model * htn, searchNode* tnSol);
pair<string,int> printTraceOfSearchNode(Model* htn, searchNode* tnSol);


}

using namespace progression;

namespace std {
template<>
struct hash<planStep> {
	size_t operator()(const planStep& ps) const {
		return ps.id;
	}
};
}

#endif /* PROGRESSIONNETWORK_H_ */
