#ifndef sog_h_INCLUDED
#define sog_h_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"


struct SOG {
	int numberOfVertices;
	vector<set<int>> adj; // successors
};

SOG* optimiseSOG(vector<int> & methods, Model* htn);


#endif
