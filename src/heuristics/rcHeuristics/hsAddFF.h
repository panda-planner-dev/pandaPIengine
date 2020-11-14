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
#include "../../intDataStructures/IntPairHeap.h"
#include "../../intDataStructures/bucketSet.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "../../intDataStructures/IntStack.h"
#include "../../Model.h"

// inner Types
typedef unsigned long hType;
#define hUnreachable ULONG_MAX;

using namespace std;

namespace progression {

    enum myHeu {
        sasAdd, sasFF
    };

    class ComparePair {
    public:
        bool operator()(pair<int, int> *n1, pair<int, int> *n2);
    };

    class hsAddFF {
    public:
        hsAddFF(Model *sas);

        virtual ~hsAddFF();

        int getHeuristicValue(bucketSet &s, noDelIntSet &g);

        Model *m;
        myHeu heuristic = sasFF;
        int calls = 0;
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

        int getFF(noDelIntSet &g, int hVal);
    };

} /* namespace progression */

#endif /* HEURISTICS_HSADDFF_H_ */
