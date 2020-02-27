//============================================================================
// Name        : SearchEngine.cpp
// Author      : Daniel Höller
// Version     :
// Copyright   : 
// Description : Search Engine for Progression HTN Planning
//============================================================================

#include "flags.h" // defines flags


#include <iostream>
#include <stdlib.h>
#include <cassert>
#include <unordered_set>

#if SEARCHTYPE == HEURISTICSEARCH
#include "./search/PriorityQueueSearch.h"
#endif

#include "Model.h"
#if HEURISTIC == RCFF
#include "heuristics/rcHeuristics/hsAddFF.h"
#include "heuristics/rcHeuristics/hsLmCut.h"
#include "heuristics/rcHeuristics/hsFilter.h"
#endif
#include "intDataStructures/IntPairHeap.h"
#include "intDataStructures/bIntSet.h"

#include "heuristics/rcHeuristics/RCModelFactory.h"

using namespace std;
using namespace progression;

int main(int argc, char *argv[]) {
	srand(42);
	string s;
	string s2;
	string s3;

	cout << "Reading HTN model..." << endl;
	if(argc >= 3){
		s = argv[1];
		s2 = argv[2];
	} else {
		s = "add local path here";
		s2 = "add local path here";
	}
#ifndef NDEBUG
	cout << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug." << endl;
#endif

/*
 * Read model
 */
	cout << "Reading HTN model..." << endl;
	Model* htn = new Model();
	htn->read(s);
	assert(htn->isHtnModel);
	searchNode* tnI = htn->prepareTNi(htn);

#ifdef MAINTAINREACHABILITY
	htn->calcSCCs();
	htn->calcSCCGraph();

	// add reachability information to initial task network
	htn->updateReachability(tnI);
#endif

/*
 * Create Search
 */
#if SEARCHTYPE == HEURISTICSEARCH
	PriorityQueueSearch search;
#endif

/*
 * Create Heuristic
 */
#if HEURISTIC == ZERO
	cout << "Heuristic: 0 for every node" << endl;
	search.hF = new hhZero(htn);
#endif
#ifdef RCHEURISTIC
	cout << "Heuristic: RC encoding" << endl;
	Model* heuristicModel = new Model();
	heuristicModel->read(s2);
#if HEURISTIC == RCFF
	search.hF = new hhRC(htn, new hsAddFF(heuristicModel));
	search.hF->sasH->heuristic = sasFF;
	cout << "- Inner heuristic: FF" << endl;
#elif HEURISTIC == RCADD
	search.hF = new hhRC(htn, new hsAddFF(heuristicModel));
	search.hF->sasH->heuristic = sasAdd;
	cout << "- Inner heuristic: Add" << endl;
#elif HEURISTIC == RCLMC
	search.hF = new hhRC(htn, new hsLmCut(heuristicModel));
	cout << "- Inner heuristic: LM-Cut" << endl;
#elif HEURISTIC == RCFILTER
	search.hF = new hhRC(htn, new hsFilter(heuristicModel));
	cout << "- Inner heuristic: Filter" << endl;
#endif
#endif
/*
 * Start Search
 */
	int timeL = TIMELIMIT;
	cout << "Time limit: " << timeL << " seconds" << endl;
	search.search(htn, tnI, timeL);
	delete htn;
#ifdef RCHEURISTIC
	delete heuristicModel;
#endif
	return 0;
}
