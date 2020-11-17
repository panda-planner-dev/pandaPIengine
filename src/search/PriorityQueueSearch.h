/*
 * PriorityQueueSearch.h
 *
 *  Created on: 10.09.2017
 *      Author: Daniel Höller
 */

#ifndef PRIORITYQUEUESEARCH_H_
#define PRIORITYQUEUESEARCH_H_
#include "../ProgressionNetwork.h"
#include "../heuristics/hhZero.h"
#include "../heuristics/rcHeuristics/hhRC.h"
#ifdef LMCOUNTHEURISTIC
#include "../heuristics/landmarks/hhLMCount.h"
#endif

namespace progression {

class PriorityQueueSearch {
public:
	PriorityQueueSearch();
	virtual ~PriorityQueueSearch();

	void search(Model* h, searchNode* tnI, int timeLimit);
#if HEURISTIC == ZERO
	hhZero *hF;
#endif
#ifdef RCHEURISTIC
	hhRC *hF;
#endif
#ifdef LMCOUNTHEURISTIC
	hhLMCount *hF;
#endif
private:
    	Model* m;
	searchNode* handleNewSolution(searchNode* newSol, searchNode* globalSolPointer, long time);
	const bool optimzeSol = OPTIMIZEUNTILTIMELIMIT;
	int foundSols = 0;
	int solImproved = 0;
	long firstSolTime = 0;
	long bestSolTime = 0;

        bool insertVisi2(searchNode *n);
    };

} /* namespace progression */

#endif /* PRIORITYQUEUESEARCH_H_ */
