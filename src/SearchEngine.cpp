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
#include <fstream>
#include <EnforcedHillClimbing.h>
#include <visitedLists/vlDummy.h>

#include "search/VisitedList.h"

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


namespace progression {
#ifdef SAVESEARCHSPACE
	ofstream* stateSpaceFile;
#endif
}

int main(int argc, char *argv[]) {
#ifdef SAVESEARCHSPACE
	stateSpaceFile = new ofstream("statespace.txt",ofstream::out);
#endif

#ifndef NDEBUG
	cout << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug." << endl;
#endif
	//srand(atoi(argv[4]));

	string s;
	int seed = 42;
	if (argc == 1) {
		cout << "No file name passed. Reading input from stdin";
		s = "stdin";
	} else {
		s = argv[1];
		if (argc > 2) seed = atoi(argv[2]);
	}
	cout << "Random seed: " << seed << endl;
	srand(seed);


/*
 * Read model
 */
	cerr << "Reading HTN model from file \"" << s << "\" ... " << endl;
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

///////////////// Create Heuristic

#if HEURISTIC == ZERO
	cout << "Heuristic: 0 for every node" << endl;
	hhZero hF (htn);
#endif
#ifdef RCHEURISTIC
	cout << "Heuristic: RC encoding" << endl;
	Model* heuristicModel;
	cout << "Computing RC model ... " << endl;
	RCModelFactory* factory = new RCModelFactory(htn);
	heuristicModel = factory->getRCmodelSTRIPS();
	delete factory;

#if HEURISTIC == RCFF
	hhRC hF (htn, new hsAddFF(heuristicModel));
	hF.sasH->heuristic = sasFF;
	cout << "- Inner heuristic: FF" << endl;
#elif HEURISTIC == RCADD
	hhRC hF (htn, new hsAddFF(heuristicModel));
	hF.sasH->heuristic = sasAdd;
	cout << "- Inner heuristic: Add" << endl;
#elif HEURISTIC == RCLMC
	hhRC hF (htn, new hsLmCut(heuristicModel));
	cout << "- Inner heuristic: LM-Cut" << endl;
#elif HEURISTIC == RCFILTER
	hhRC hF (htn, new hsFilter(heuristicModel));
	cout << "- Inner heuristic: Filter" << endl;
#endif
#endif

	

#if SEARCHTYPE == DFSEARCH
	StackFringe fringe;
#elif SEARCHTYPE == BFSEARCH
	QueueFringe fringe;
#else
#ifdef PREFMOD
	cout << "- using preferred modifications and an alternating fringe." << endl;
	AlternatingFringe fringe;
	// fringe push is different ---> macro! fringe.push(tnI, true);
#else // SEARCHTYPE == HEURISTICSEARCH
	cout << "- using priority queue as fringe." << endl;
	priority_queue<searchNode*, vector<searchNode*>, CmpNodePtrs> fringe;
#endif
#endif

	int timeL = TIMELIMIT;
	cout << "Time limit: " << timeL << " seconds" << endl;


	VisitedList visi (htn);


    searchNode* tnI2 = htn->prepareTNi(htn);
	
	// create search object and start	

#if SEARCHTYPE == HEURISTICSEARCH
	PriorityQueueSearch search;
	search.search(htn, tnI2, timeL, hF, visi, fringe);
#endif


/*
    vlDummy d;

	EnforcedHillClimbing<hhRC,vlDummy> ehc;
    hhRC *h = new hhRC(htn, new hsAddFF(heuristicModel));
    h->sasH->heuristic = sasFF;
    ehc.search(htn, tnI, timeL, h, new vlDummy);
*/
	delete htn;
#ifdef RCHEURISTIC
	delete heuristicModel;
#endif
	return 0;
}
