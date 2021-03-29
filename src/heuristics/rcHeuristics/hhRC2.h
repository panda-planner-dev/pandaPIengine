//
// Created by dh on 10.03.20.
//

#ifndef PANDAPIENGINE_HHRC2_H
#define PANDAPIENGINE_HHRC2_H

#include <set>
#include <forward_list>
#include <Heuristic.h>
#include "../../Model.h"
#include "../../intDataStructures/bucketSet.h"
#include "../../intDataStructures/bIntSet.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "hsAddFF.h"
#include "hsLmCut.h"
#include "hsFilter.h"
#include "RCModelFactory.h"

enum innerHeuristic {FILTER, ADD, FF, LMCUT};

class hhRC2 : public Heuristic {
private:
    noDelIntSet gset;
    noDelIntSet intSet;
	bucketSet s0set;

public:
#if (HEURISTIC == RCFF2 || HEURISTIC == RCADD2)
    hsAddFF* sasH;
#elif (HEURISTIC == RCFILTER2)
	hsFilter* sasH;
#endif

#if (HEURISTIC == RCLMC2)
    hsLmCut* sasH;
#else
#ifdef RCLMC2STORELMS
    hsLmCut* sasH;
#endif
#endif

    hhRC2(Model* htnModel);
    const Model* htn;
    RCModelFactory* factory;
    virtual ~hhRC2();
    void setHeuristicValue(searchNode *n, searchNode *parent, int action) override;
    void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) override;

    const bool storeCuts = true;
    IntUtil iu;
    list<LMCutLandmark *>* cuts = new list<LMCutLandmark *>();

    int setHeuristicValue(searchNode *n);
};


#endif //PANDAPIENGINE_HHRC2_H
