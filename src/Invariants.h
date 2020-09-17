#ifndef invariants_INCLUDED
#define invariants_INCLUDED

#include "flags.h" // defines flags
#include "Model.h"

void extract_invariants_from_parsed_model(Model * htn);
#ifdef RINTANEN_INVARIANTS
void compute_Rintanen_Invariants(Model * htn);
#endif

bool can_state_features_co_occur(Model * htn, int a, int b);

extern unordered_set<int> * binary_invariants;

#endif
