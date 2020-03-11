//
// Created by dh on 28.02.20.
//

#ifndef PANDAPIENGINE_LMAONODE_H
#define PANDAPIENGINE_LMAONODE_H

#include "IntUtil.h"

enum andOrNodeType {INIT, AND, OR};

class LmAoNode {

public:
    LmAoNode(int ownIndex);

    // node information
    int ownIndex;
    bool containsFullSet; // marks nodes that contain all lms (this is set initially)
    int maxSize; // size of full lm set
    andOrNodeType nodeType;

    // landmarks of this node
    int numLMs = 0; // current number of lms
    int lmContainerSize = 0; // size of lms -> when I find more, container size needs to be increased
    int* lms = nullptr;

    // graph stored via adjacency lists (predecessors and successors)
    int  numAffectedNodes = 0;
    int* affectedNodes;
    int numAffectedBy = 0;
    int* affectedBy;

    int getSize();
    bool contains(int i);
    void print();

private:
    IntUtil iu;
};


#endif //PANDAPIENGINE_LMAONODE_H
