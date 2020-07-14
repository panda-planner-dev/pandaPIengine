//============================================================================
// Name        : SearchEngine.cpp
// Author      : Daniel Höller
// Version     :
// Copyright   :
// Description : Search Engine for Progression HTN Planning
//============================================================================

#include "flags.h" // defines flags

#include <cassert>
#include <iostream>
#include <stdlib.h>
#include <unordered_set>

#if SEARCHTYPE == HEURISTICSEARCH
#include "./search/PriorityQueueSearch.h"
#endif

#include "Model.h"
#include "debug.h"
#if HEURISTIC == RCFF
#include "heuristics/rcHeuristics/hsAddFF.h"
#include "heuristics/rcHeuristics/hsFilter.h"
#include "heuristics/rcHeuristics/hsLmCut.h"
#endif
#include "intDataStructures/IntPairHeap.h"
#include "intDataStructures/bIntSet.h"

#include "heuristics/rcHeuristics/RCModelFactory.h"

#include "symbolic_search/automaton.h"
#include "symbolic_search/sym_variables.h"
#include "symbolic_search/transition_relation.h"

using namespace std;
using namespace progression;

int main(int argc, char *argv[]) {
#ifndef NDEBUG
  cout << "You have compiled the search engine without setting the NDEBUG "
          "flag. This will make it slow and should only be done for debug."
       << endl;
#endif
  // srand(atoi(argv[4]));

  setDebugMode(true);
  string s;
  int seed = 42;
  if (argc == 1) {
    cout << "No file name passed. Reading input from stdin";
    s = "stdin";
  } else {
    s = argv[1];
    if (argc > 2)
      seed = atoi(argv[2]);
  }
  cout << "Random seed: " << seed << endl;
  srand(seed);

  /*
   * Read model
   */
  cerr << "Reading HTN model from file \"" << s << "\" ... " << endl;
  Model *htn = new Model();
  htn->read(s);
  assert(htn->isHtnModel);
  searchNode *tnI = htn->prepareTNi(htn);
  
  build_automaton(htn);

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
  Model *heuristicModel;
  cout << "Computing RC model ... " << endl;
  RCModelFactory *factory = new RCModelFactory(htn);
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


  //return 0;


  int timeL = TIMELIMIT;
  cout << "Time limit: " << timeL << " seconds" << endl;
  search.search(htn, tnI, timeL);
  delete htn;
#ifdef RCHEURISTIC
  delete heuristicModel;
#endif
  return 0;
}
