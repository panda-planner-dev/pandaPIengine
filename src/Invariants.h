#ifndef invariants_INCLUDED
#define invariants_INCLUDED

#include "flags.h" // defines flags
#include "Model.h"

void extract_invariants_from_parsed_model(Model * htn);
int count_invariants(Model * htn);


void compute_Rintanen_initial_invariants(Model * htn,
		vector<pair<int,int>> & v0,
		bool * & toDelete,
		vector<vector<int>> & posInvarsPerPredicate,
		vector<vector<int>> & negInvarsPerPredicate
	);


bool compute_Rintanten_action_applicable(Model * htn,
		int tIndex,
		vector<pair<int,int>> & v0,
		bool * & toDelete,
		vector<vector<int>> & posInvarsPerPredicate,
		vector<vector<int>> & negInvarsPerPredicate,
		bool * & posInferredPreconditions,
		bool * & negInferredPreconditions
	);

bool compute_Rintanten_action_effect(Model * htn,
		int tIndex,
		vector<pair<int,int>> & v0,
		bool * & toDelete,
		vector<vector<int>> & posInvarsPerPredicate,
		vector<vector<int>> & negInvarsPerPredicate,
		bool * & posInferredPreconditions,
		bool * & negInferredPreconditions
	);

void compute_Rintanen_reduce_invariants(Model * htn,
		vector<pair<int,int>> & v0,
		bool * & toDelete,
		vector<vector<int>> & posInvarsPerPredicate,
		vector<vector<int>> & negInvarsPerPredicate
	);

	
void compute_Rintanen_Invariants(Model * htn);

bool can_state_features_co_occur(Model * htn, int a, int b);

extern unordered_set<int> * binary_invariants;

#endif
