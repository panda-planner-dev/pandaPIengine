//
// Created by dh on 10.03.20.
//

#ifndef PANDAPIENGINE_HHRC2_H
#define PANDAPIENGINE_HHRC2_H

#include <set>
#include <forward_list>
#include "../../Model.h"
#include "../../intDataStructures/bucketSet.h"
#include "../../intDataStructures/bIntSet.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "hsAddFF.h"
#include "hsLmCut.h"
#include "hsFilter.h"
#include "RCModelFactory.h"

enum innerHeuristic {FILTER, ADD, FF, LMCUT};

class hhRC2 {
private:
    noDelIntSet gset;
    noDelIntSet intSet;
	bucketSet s0set;

    void setHeuristicValue(searchNode *n);
public:
#if (HEURISTIC == RCFF2 || HEURISTIC == RCADD2)
    hsAddFF* sasH;
#elif (HEURISTIC == RCLMC2)
    hsLmCut* sasH;
#else
	hsFilter* sasH;
#endif

    hhRC2(Model* htnModel);
    const Model* htn;
    RCModelFactory* factory;
    virtual ~hhRC2();
    void setHeuristicValue(searchNode *n, searchNode *parent, int action);
    void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method);

    const bool storeCuts = true;
    IntUtil iu;
    list<LMCutLandmark *>* cuts = new list<LMCutLandmark *>();
};


#endif //PANDAPIENGINE_HHRC2_H
