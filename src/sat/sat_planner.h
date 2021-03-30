#ifndef sat_planner_h_INCLUDED
#define sat_planner_h_INCLUDED


// do NOT change the ordering of the following includes
#include "../flags.h" // defines flags
//#include "Config.h" // defines a configuration
//#include "Autoconf.h" // sets automatic flags based on the configuration

#include "../Model.h"

void solve_with_sat_planner(Model * htn);

#endif
