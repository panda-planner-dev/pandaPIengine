#ifndef sat_planner_h_INCLUDED
#define sat_planner_h_INCLUDED


// do NOT change the ordering of the following includes
#include "../flags.h" // defines flags
//#include "Config.h" // defines a configuration
//#include "Autoconf.h" // sets automatic flags based on the configuration

#include "../Model.h"

enum sat_pruning{
	SAT_NONE=0, SAT_FF=1, SAT_H2=2
};

void solve_with_sat_planner(Model * htn, bool block_compression, bool sat_mutexes, sat_pruning pruningMode, bool effectLessActionsInSeparateLeaf, bool optimise);

#endif
