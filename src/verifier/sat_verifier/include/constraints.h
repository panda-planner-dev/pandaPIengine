//
// Created by u6162630 on 4/3/23.
//

#ifndef _CONSTRAINTS_H_
#define _CONSTRAINTS_H_
#include "variables.h"
#include "sog.h"
#include "execution.h"
#include "sat_encoder.h"

class ConstraintsOnMapping {
public:
    ConstraintsOnMapping(
            void *solver,
            sat_capsule &capsule,
            vector<int> &plan,
            PlanToSOGVars *mapping,
            SOG *sog);
};

class ConstraintsOnStates {
public:
    ConstraintsOnStates(
            void *solver,
            Model *htn,
            StateVariables *vars,
            PlanExecution *execution);
};

class ConstraintsOnSequence {
public:
    ConstraintsOnSequence(
            void *solver,
            Model *htn,
            int length,
            SOG *sog,
            StateVariables *stateVars,
            PlanToSOGVars *mapping) {
        for (int pos = 0; pos < length; pos++)
            for (int a = 0; a < htn->numActions; a++) {
                int var = mapping->getSequenceVar(pos, a);
                if (var == -1) continue;
                for (int i = 0; i < htn->numPrecs[a]; i++) {
                    int prop = htn->precLists[a][i];
                    int propVar = stateVars->get(pos, prop);
                    implies(solver, var, propVar);
                }
            }
    }
};
#endif
