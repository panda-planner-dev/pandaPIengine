#ifndef sog_h_INCLUDED
#define sog_h_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"


struct SOG {
	int numberOfVertices;
	vector<unordered_set<uint32_t>> labels;

	vector<unordered_set<uint16_t>> adj; // successors
	vector<unordered_set<uint16_t>> bdj; // predecessors

	vector<vector<uint16_t>> methodSubTasksToVertices;
};

SOG* optimiseSOG(vector<tuple<uint32_t,uint32_t,uint32_t>> & methods, Model* htn);


#endif
