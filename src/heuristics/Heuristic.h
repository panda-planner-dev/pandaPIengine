//
// Created by dh on 29.03.21.
//

#ifndef PANDAPIENGINE_HEURISTIC_H
#define PANDAPIENGINE_HEURISTIC_H

#include "../Model.h"

class Heuristic {
protected:
    int index;
    Model* htn;
public:
    Heuristic(Model* htnModel, int index);
    
	// returns textual description of the heuristic for output 
	virtual string getDescription() = 0;

    virtual void setHeuristicValue(searchNode *n, searchNode *parent, int action) = 0;
    virtual void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) = 0;
};


#endif //PANDAPIENGINE_HEURISTIC_H
