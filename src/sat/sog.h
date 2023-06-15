#ifndef sog_h_INCLUDED
#define sog_h_INCLUDED

#include <stdint.h>
#include <fstream>
#include "../flags.h" // defines flags
#include "../Model.h"

struct PDT;

struct SOG {
	int numberOfVertices;
	vector<unordered_set<uint32_t>> labels;

	vector<unordered_set<uint16_t>> adj; // successors
	vector<unordered_set<uint16_t>> bdj; // predecessors

	vector<vector<uint16_t>> methodSubTasksToVertices;

	void printDot(Model * htn, ofstream & dfile);

	SOG* expandSOG(vector<SOG*> nodeSOGs);
	void removeTransitiveOrderings();


	vector<PDT*> leafOfNode;
	vector<unordered_set<int>> successorSet;
	void succdfs(int n, vector<bool> &visi);
	void calcSucessorSets();
	
	vector<unordered_set<int>> predecessorSet;
	void precdfs(int n, vector<bool> &visi);
	void calcPredecessorSets();

	vector<int> firstPossible;
	vector<int> lastPossible;
	vector<bool> leafContainsEffectAction;
};

SOG* optimiseSOG(vector<tuple<uint32_t,uint32_t,uint32_t>> & methods, Model* htn, bool effectLessActionsInSeparateLeaf);

SOG* generateSOGForLeaf(PDT* leaf);

#endif
