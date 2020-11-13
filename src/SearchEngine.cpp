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

#include "Debug.h"
#include "Model.h"
#if HEURISTIC == RCFF
#include "heuristics/rcHeuristics/hsAddFF.h"
#include "heuristics/rcHeuristics/hsLmCut.h"
#include "heuristics/rcHeuristics/hsFilter.h"
#endif
#include "intDataStructures/IntPairHeap.h"
#include "intDataStructures/bIntSet.h"

#include "heuristics/rcHeuristics/RCModelFactory.h"

#include "sat/sat_planner.h"
#include "sat/disabling_graph.h"

#include "Invariants.h"

using namespace std;
using namespace progression;

struct test {
	bool a:1;
	unsigned b : 31, c:32;
};

int main(int argc, char *argv[]) {
#ifndef NDEBUG
	cout << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug." << endl;
#endif
	//srand(atoi(argv[4]));


	cout << sizeof (test) << endl;
	cout << sizeof (uint64_t) << endl;
	cout << sizeof (tuple<bool,uint32_t,uint32_t>) << endl;
	cout << sizeof (tuple<int16_t,bool,uint32_t,uint32_t>) << endl;
	cout << sizeof (tuple<int,bool,int,int>) << endl;

	//return 0;

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

	setDebugMode(true);
	extract_invariants_from_parsed_model(htn);
#ifdef RINTANEN_INVARIANTS
#ifdef SAT_USEMUTEXES
	compute_Rintanen_Invariants(htn);
#endif
#endif

	solve_with_sat_planner(htn);
	return 0;

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
	Model* heuristicModel;
	cout << "Computing RC model ... " << endl;
	RCModelFactory* factory = new RCModelFactory(htn);
	heuristicModel = factory->getRCmodelSTRIPS();
	delete factory;

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
