/*
 * hhDOfree.h
 *
 *  Created on: 27.09.2018
 *      Author: Daniel Höller
 */

#ifndef HEURISTICS_HHDOFREE_H_
#define HEURISTICS_HHDOFREE_H_

#include <math.h>
#include <sys/time.h>
#include "../../Model.h"
#include "../../ProgressionNetwork.h"
#include "../../intDataStructures/noDelIntSet.h"
#include <iostream>
#include <vector>
#include <string>
#include <limits.h>
#include <set>
#include <ilcplex/ilocplex.h>
#include "../planningGraph.h"
#include "../landmarks/lmExtraction/LmCausal.h"
#include "../../intDataStructures/IntUtil.h"

// how to create the model? default -> recreate
#ifndef DOFMODE
#define DOFMODE DOFRECREATE
#endif

namespace progression {

    enum csTdg {cTdgFull, cTdgAllowUC, cTdgNone};
    enum csPg {cPgFull, cPgTimeRelaxed, cPgNone};
    enum csAndOrLms {cAndOrLmsFull, cAndOrLmsOnlyTnI, cAndOrLmsNone};
    enum csAddExternalLms {csAddExternalLmsYes, csAddExternalLmsNo};
    enum csLmcLms {cLmcLmsFull, cLmcLmsNone};
    enum csNetChange {cNetChangeFull, cNetChangeNone};

    class hhDOfree {
        Model *htn;
        //void countTNI(searchNode* n, Model* htn);
        IntUtil iu;

        // heuristic configuration
        const csTdg cTdg;
        const csPg cPg;
        const csAndOrLms cAndOrLms;
        const csLmcLms cLmcLms;
        const csAddExternalLms cAddExternalLms;
        const csNetChange cNetChange;

        int *iUF;
        int *iUA;
        int *iM;
        int *iE;
        int *iTA;
        int *iTP;
        int *EStartI;
        int *EInvSize;
        int **iEInvEffIndex;
        int **iEInvActionIndex;

        int recreateModel(searchNode *n);

        planningGraph *pg;
        noDelIntSet preProReachable;

        void updatePG(searchNode *n);

        hhRC2 *hRC = nullptr;
        LmCausal *causalLMs = nullptr;


#ifdef DOFINREMENTAL

        bool* ILPcurrentFactReachability;
        IloExtractable* factReachability;

        bool* ILPcurrentTaskReachability;
        IloExtractable* taskReachability;

        // variable indices like above
        int* iS0;
        int* iTNI;

        // cplex stuff
        IloEnv lenv;
        IloModel model;
        IloNumVarArray v;
        IloCplex cplex;

        // formulae setting the values
        IloExtractable* initialState;
        IloExtractable* setStateBits;
        IloExtractable* initialTasks;

        // how it is set in the current model
        int* ILPcurrentS0;
        int* ILPcurrentTNI;

        void updateS0(searchNode *n);
        void updateTNI(searchNode *n);
        void updateRechability(searchNode *n);

#endif

        // mappings for the scc reachability, these are static
        int **sccNumIncommingMethods = nullptr;
        int **sccNumInnerFromTo = nullptr;
        int **sccNumInnerToFrom = nullptr;
        int ***sccNumInnerFromToMethods = nullptr;

        int ***sccIncommingMethods = nullptr; // methods from another scc reaching a certain scc
        int ***sccInnerFromTo = nullptr; // maps a tasks to those that it may be decomposed into
        int ***sccInnerToFrom = nullptr; // maps a tasks to those that may be decomposed into it
        int ****sccInnerFromToMethods = nullptr;

        // data structures for the indices
        int *RlayerPrev = nullptr;
        int *RlayerCurrent = nullptr;
        int **Ivars = nullptr;

    public:
        hhDOfree(Model *htn, searchNode *n);

        virtual ~hhDOfree();

        void setHeuristicValue(searchNode *n, searchNode *parent, int action);

        void setHeuristicValue(searchNode *n, searchNode *parent, int absTask,
                               int method);

    private:
        const int largeC = 100;

        void printHeuristicInformation(Model *htn);

        int* numAP;
        int* numSP;
        int* numAC;
        // int* numSC;
        int** apList;
        int** spList;
        int** acList;
        // int** scList;

        int* vOfL;
    };

} /* namespace progression */

#endif /* HEURISTICS_HHDOFREE_H_ */

