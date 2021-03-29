//
// Created by dh on 29.03.21.
//

#ifndef PANDAPIENGINE_ONEQUEUEWASTARFRINGE_H
#define PANDAPIENGINE_ONEQUEUEWASTARFRINGE_H

#include "../../ProgressionNetwork.h"
#include <queue>

enum aStar {gValNone, gValPathCosts, gValActionCosts};

class OneQueueWAStarFringe {
    priority_queue<searchNode*, vector<searchNode*>, CmpNodePtrs> fringe;
public:
    OneQueueWAStarFringe(aStar AStarOptions, int hWeight);
    bool isEmpty();
    searchNode* pop();
    void push(searchNode* n);
    int size();
};


#endif //PANDAPIENGINE_ONEQUEUEWASTARFRINGE_H
