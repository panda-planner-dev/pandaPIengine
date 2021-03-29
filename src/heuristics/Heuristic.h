//
// Created by dh on 29.03.21.
//

#ifndef PANDAPIENGINE_HEURISTIC_H
#define PANDAPIENGINE_HEURISTIC_H

#include "../Model.h"

class Heuristic {
public:
    Heuristic(Model* htnModel);

    virtual void setHeuristicValue(searchNode *n, searchNode *parent, int action) = 0;
    virtual void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) = 0;
};


#endif //PANDAPIENGINE_HEURISTIC_H
