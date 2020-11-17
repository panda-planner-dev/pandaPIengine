/*
 * ProgressionNetwork.cpp
 *
 *  Created on: 25.09.2017
 *      Author: Daniel Höller
 */

#include "ProgressionNetwork.h"
#include <stdlib.h>

namespace progression {

#ifdef TRACESOLUTION
int currentSolutionStepInstanceNumber = 0;
#endif

#ifdef SAVESEARCHSPACE
int currentSearchNodeID = 0;
#endif 

////////////////////////////////
// solutionStep
////////////////////////////////
solutionStep::~solutionStep() {
	if (prev != nullptr) {
		prev->pointersToMe--;
		if (prev->pointersToMe == 0) {
			delete prev;
		}
	}
}
////////////////////////////////
// planStep
////////////////////////////////

bool planStep::operator==(const planStep &that) const {
	return (this->id == that.id);
}

planStep::~planStep() {
	for (int i = 0; i < numSuccessors; i++) {
		planStep* succ = successorList[i];
		succ->pointersToMe--;
		if (succ->pointersToMe == 0) {
			delete succ;
		}
	}
	delete[] successorList;
#ifdef MAINTAINREACHABILITY
	delete[] reachableT;
#endif
#ifdef RCHEURISTIC
	delete[] goalFacts;
#endif
}

////////////////////////////////
// searchNode
////////////////////////////////

bool searchNode::operator<(searchNode other) const {
	return heuristicValue > other.heuristicValue;
}

searchNode::searchNode() {
	hRand = rand();
	modificationDepth = -1;
	mixedModificationDepth = -1;
	heuristicValue = -1;
	unconstraintPrimitive = nullptr;
	unconstraintAbstract = nullptr;
	numAbstract = 0;
	numPrimitive = 0;
	solution = nullptr;
	searchNodeID = currentSearchNodeID++;
}

searchNode::~searchNode() {
	for (int i = 0; i < numAbstract; i++) {
		unconstraintAbstract[i]->pointersToMe--;
		if (unconstraintAbstract[i]->pointersToMe == 0) {
			delete unconstraintAbstract[i];
		}
	}
	for (int i = 0; i < numPrimitive; i++) {
		unconstraintPrimitive[i]->pointersToMe--;
		if (unconstraintPrimitive[i]->pointersToMe == 0) {
			delete unconstraintPrimitive[i];
		}
	}
	if (solution != nullptr) {
		solution->pointersToMe--;
		if (solution->pointersToMe == 0) {
			delete solution;
		}
	}
	delete[] unconstraintAbstract;
	delete[] unconstraintPrimitive;

#ifdef TRACKTASKSINTN
	delete[] containedTasks;
	delete[] containedTaskCount;
#endif

#if STATEREP == SRLIST
	delete[] state;
#endif
}

////////////////////////////////
// CmpNodePtrs
////////////////////////////////

bool CmpNodePtrs::operator()(const searchNode* a, const searchNode* b) const {
	if (a->heuristicValue == b->heuristicValue)
		return a->hRand > b->hRand;
	return a->heuristicValue > b->heuristicValue;
}

}
