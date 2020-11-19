#ifndef PANDAPIENGINE_VISITEDLIST_H
#define PANDAPIENGINE_VISITEDLIST_H

#include "../ProgressionNetwork.h"

using namespace progression;

struct VisitedList{
	VisitedList(Model * m);
	
	// insert the node into the visited list
	// @returns true if the node was *new* and false, if the node was already contained in the visited list
	bool insertVisi(searchNode * node);

	bool canDeleteProcessedNodes;
	
	int attemptedInsertions = 0;
	int uniqueInsertions = 0;
	int subHashCollision = 0;
	double time = 0;

private:
	Model * htn;
	bool useTotalOrderMode;

#if (TOVISI == TOVISI_SEQ)
#ifndef SAVESEARCHSPACE 
	map<vector<uint64_t>, set<vector<int>>> visited;
#else
	map<vector<uint64_t>, map<vector<int>,int>> visited;
#endif
#elif (TOVISI == TOVISI_PRIM)
	map<vector<uint64_t>, unordered_set<int>> visited;
#elif (TOVISI == TOVISI_PRIM_EXACT)
	map<vector<uint64_t>, map<int,set<vector<int>>>> visited;
#endif

	map<tuple< vector<uint64_t>, map<int,map<int,int>>, set<pair<int,int>> > , vector<searchNode*> > po_occ;


};


#endif
