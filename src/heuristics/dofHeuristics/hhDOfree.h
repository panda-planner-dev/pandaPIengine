/*
 * hhDOfree.h
 *
 *  Created on: 27.09.2018
 *      Author: Daniel Höller
 */

#ifndef HEURISTICS_HHDOFREE_H_
#define HEURISTICS_HHDOFREE_H_

//#define OUTPUTLPMODEL
//#define NAMEMODEL // assign human-readable names to each ILP variable

#include <rcHeuristics/hhRC2.h>
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
#include <string>
#include <ilcplex/ilocplex.h>
#include "../planningGraph.h"
#include "../landmarks/lmExtraction/LmCausal.h"
#include "../../intDataStructures/IntUtil.h"
#include "../rcHeuristics/hhRC2.h"

namespace progression {

    enum csSetting {
        cSatisficing, cOptimal
    };
    enum csTdg {
        cTdgFull, cTdgAllowUC, cTdgNone
    };
    enum csPg {
        cPgFull, cPgTimeRelaxed, cPgNone
    };
    enum csAndOrLms {
        cAndOrLmsNone, cAndOrLmsOnlyTnI, cAndOrLmsFull
    };
    enum csAddExternalLms {
        cAddExternalLmsYes, cAddExternalLmsNo
    };
    enum csLmcLms {
        cLmcLmsFull, cLmcLmsNone
    };
    enum csNetChange {
        cNetChangeFull, cNetChangeNone
    };

    class hhDOfree : public Heuristic {
        //int fileID = 0;

        Model *htn;
        //void countTNI(searchNode* n, Model* htn);
        IntUtil iu;

        // heuristic configuration
        const IloNumVar::Type cIntType;
        const IloNumVar::Type cBoolType;
        const csSetting cSetting;
        const csTdg cTdg;
        const csPg cPg;
        const csAndOrLms cAndOrLms;
        const csLmcLms cLmcLms;
        const csNetChange cNetChange;
        const csAddExternalLms cAddExternalLms;

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

        hhRC2<hsLmCut> *hRC = nullptr;
        LmCausal *causalLMs = nullptr;

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
        hhDOfree(Model *htn, searchNode *n, int index, IloNumVar::Type IntType, IloNumVar::Type BoolType, csSetting IlpSetting, csTdg tdgConstrs, csPg pgConstrs, csAndOrLms aoLMConstrs, csLmcLms lmcLMConstrs, csNetChange ncConstrs, csAddExternalLms addLMConstrs);

        virtual ~hhDOfree();
		
		string getDescription(){
			return string("hhDOf(") + 
        "intType=" + (cIntType == IloNumVar::Int?"int":"float") + ";" + 
        "boolType=" + (cBoolType == IloNumVar::Bool?"bool":"float") + ";" + 
        "setting=" + (cSetting == cOptimal?"optimal":"satisficing") + ";" + 
        "tdg=" + (cTdg == cTdgFull?"full":(cTdg==cTdgAllowUC?"with-uc":"none")) + ";" + 
        "pg=" + (cPg == cPgFull?"full":(cPg==cPgTimeRelaxed?"time-relaxed":"none")) + ";" + 
        "andOrLms=" + (cAndOrLms == cAndOrLmsFull?"full":(cAndOrLms==cAndOrLmsOnlyTnI?"onlyTNi":"none")) + ";" + 
        "externalLms=" + (cAddExternalLms == cAddExternalLmsYes?"yes":"no") + ";" + 
        "lmcLms=" + (cLmcLms == cLmcLmsFull?"full":"no") + ";" + 
        "netchange=" + (cNetChange == cNetChangeFull?"full":"no") + 
		")";}

        void setHeuristicValue(searchNode *n, searchNode *parent, int action) override;

        void setHeuristicValue(searchNode *n, searchNode *parent, int absTask,
                               int method) override;

        void printHeuristicInformation();

    private:
        const int largeC = 100;

        int *numAP;
        int *numSP;
        int *numAC;
        // int* numSC;
        int **apList;
        int **spList;
        int **acList;
        // int** scList;

        int *vOfL;
    };
} /* namespace progression */

#endif /* HEURISTICS_HHDOFREE_H_ */

