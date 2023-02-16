#ifndef partial_order_h_INCLUDED
#define partial_order_h_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"
#include "pdt.h"


struct MatchingData{
	vector<vector<int>> matchingPerLeaf;
	vector<vector<int>> matchingPerPosition;
	vector<vector<int>> matchingPerPositionAMO;
	
	vector<vector<pair<int,int>>> vars;

	SOG * leafSOG;
};


void generate_matching_formula(void* solver, sat_capsule & capsule, Model * htn, SOG* leafSOG, vector<vector<pair<int,int>>> & vars, MatchingData & matching);

#endif
