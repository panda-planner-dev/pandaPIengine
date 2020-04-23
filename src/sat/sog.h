#ifndef sog_h_INCLUDED
#define sog_h_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"


struct SOG {
	int numberOfVertices;
	vector<unordered_set<int>> labels;

	vector<unordered_set<int>> adj; // successors
	vector<unordered_set<int>> bdj; // predecessors

	vector<vector<int>> methodSubTasksToVertices;
};

SOG* optimiseSOG(vector<tuple<int,int,int>> & methods, Model* htn);


#endif
