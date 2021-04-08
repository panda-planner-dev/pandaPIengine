/*
 * sAddFF.h
 *
 *  Created on: 23.09.2017
 *      Author: Daniel Höller
 */

#ifndef HEURISTICS_HSADDFF_H_
#define HEURISTICS_HSADDFF_H_

#include <climits>
#include <utility>
#include <list>
#include "../../intDataStructures/IntPairHeap.h"
#include "../../intDataStructures/bucketSet.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "../../intDataStructures/IntStack.h"
#include "../../Model.h"
#include "LMCutLandmark.h"

// inner Types
//typedef long long hType;
//#define hUnreachable  LONG_LONG_MAX
typedef int hType;
#define hUnreachable  INT_MAX

using namespace std;

namespace progression {

    enum myHeu {
        sasAdd, sasFF
    };

    class hsAddFF {
    public:
        hsAddFF(Model *sas);

        virtual ~hsAddFF();

        int getHeuristicValue(bucketSet &s, noDelIntSet &g);
        bool reportedOverflow = false;
		
		string getDescription(){ if (heuristic == sasFF) return "ff"; else return "add";}

        Model *m;
        myHeu heuristic = sasFF;
        list<LMCutLandmark *>* cuts = new list<LMCutLandmark *>();
    private:
        // todo: when parallelized, this must be per core
        IntPairHeap<hType> *queue;
        hType *hValPropInit;

        int *numSatPrecs;
        hType *hValOp;
        hType *hValProp;
        int *reachedBy;

        noDelIntSet markedFs;
        noDelIntSet markedOps;
        IntStack needToMark;

        bool allActionsCostOne = false;

        hType getFF(noDelIntSet &g);

        bool assertPrecAddDelSets();
    };

} /* namespace progression */

#endif /* HEURISTICS_HSADDFF_H_ */
