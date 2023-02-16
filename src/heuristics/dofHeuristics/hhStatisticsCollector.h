//
// Created by dh on 01.04.20.
//

#ifndef PANDAPIENGINE_HHSTATISTICSCOLLECTOR_H
#define PANDAPIENGINE_HHSTATISTICSCOLLECTOR_H

#include "../rcHeuristics/hhRC2.h"
#include "hhDOfree.h"
#include "../../Model.h"
#include <vector>

namespace progression {

    class hhStatisticsCollector {
    public:
        hhStatisticsCollector(Model *htn, searchNode *n, int depth);

        void setHeuristicValue(searchNode *n);

        void setHeuristicValue(searchNode *n, searchNode *parent, int action);

        void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method);

        int maxDepth;
        Model *m;

        // heuristics
        hhRC2<hsLmCut> *hRcLmc;
        std::vector<hhDOfree*> ilpHs;
    };
}

#endif //PANDAPIENGINE_HHSTATISTICSCOLLECTOR_H
