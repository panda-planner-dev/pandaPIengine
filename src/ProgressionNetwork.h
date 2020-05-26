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
#include <iostream>
#include <forward_list>
#include "heuristics/landmarks/lmDataStructures/landmark.h"
#include "heuristics/landmarks/lmDataStructures/lookUpTab.h"

using namespace std;

namespace progression {

#ifdef TRACESOLUTION
extern int currentSolutionStepInstanceNumber;
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
#ifdef MAINTAINREACHABILITY
	int numReachableT;
	int* reachableT = nullptr;
#endif

#ifdef RCHEURISTIC
	int numGoalFacts;
	int* goalFacts = nullptr;
#endif

	bool operator==(const planStep &that) const;

	~planStep();
};

struct searchNode {
#if STATEREP == SRCOPY
	vector<bool> state;
#elif STATEREP == SRLIST
	int* state = nullptr;
	int stateSize;
#endif
	int numAbstract;
	int numPrimitive;
	planStep** unconstraintAbstract;
	planStep** unconstraintPrimitive;

	int heuristicValue;
	bool goalReachable = true;
	int modificationDepth;
	int mixedModificationDepth;
	int actionCosts = 0;

	solutionStep* solution;

	bool operator<(searchNode other) const;

	~searchNode();
	searchNode();

	int hRand;

#ifdef TRACKTASKSINTN
	int numContainedTasks = -1;
	int* containedTasks = nullptr;
	int* containedTaskCount = nullptr;
#endif

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

};

struct CmpNodePtrs {
	bool operator()(const searchNode* a, const searchNode* b) const;
};

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
