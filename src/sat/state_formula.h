#ifndef state_formula_h_INCLUDED
#define state_formula_h_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"
#include "sat_encoder.h"
#include "pdt.h"

// vector [t][a] where t is the timestep and the inner vector contains pair [varID, actionID]
void generate_state_transition_formula(void* solver, sat_capsule & capsule, vector<vector<pair<int,int>>> & actionVariables, vector<int> & block_base_variables, Model* htn);

void generate_state_transition_formula(void* solver, sat_capsule & capsule, vector<vector<pair<int,int>>> & actionVariables, vector<int> & block_base_variables, vector<vector<int>> & blocks, Model* htn);

void generate_mutex_formula(void* solver, sat_capsule & capsule, vector<int> & block_base_variables, unordered_set<int>* & after_leaf_invariants, Model* htn);
void generate_mutex_formula(void* solver, sat_capsule & capsule, vector<int> & block_base_variables, vector<vector<int>> & blocks, unordered_set<int>* & after_leaf_invariants, Model* htn);


void no_abstract_in_leaf(void* solver, vector<PDT*> & leafs, Model* htn);


void get_linear_state_atoms(sat_capsule & capsule, vector<PDT*> & leafs, vector<vector<pair<int,int>>> & ret);
void get_partial_state_atoms(sat_capsule & capsule, Model * htn, SOG * sog, vector<vector<pair<int,int>>> & ret, bool effectLessActionsInSeparateLeaf);

#endif
