//
// Created by dh on 28.02.20.
//

#ifndef PANDAPIENGINE_LMCAUSAL_H
#define PANDAPIENGINE_LMCAUSAL_H

#include <planningGraph.h>
#include "../../../Model.h"
#include "LmAoNode.h"
#include "noDelIntSet.h"

namespace progression {

    class LmCausal {
    public:
        LmCausal(Model *htn);

        virtual ~LmCausal();

        void calcLMs(searchNode *n, planningGraph *pg);

        void calcLMs(searchNode *n);

        void prettyprintAndOrGraph();

        // generated landmarks
        int numLMs = 0;
        landmark **landmarks;

        int getNumLMs();

        landmark **getLMs();

        void prettyPrintLMs();

    private:
        const bool printDebugInfo = false;
        const bool beTotallySilent = false;

        // actual calculation
        void pCalcLMs(searchNode *n);

        // used data
        Model *htn;
        planningGraph *pg;

        // graph (these are the nodes, each node contains an adjacency list)
        int numNodes;
        LmAoNode **nodes;

        // index operations
        int fNode(int i);

        int tNode(int i);

        int mNode(int i);

        int nodeToF(int i);

        int nodeToT(int i);

        int nodeToM(int i);

        bool isFNode(int i);

        bool isTNode(int i);

        bool isMNode(int i);

        IntPairHeap *heap; // used for the main loop
        IntPairHeap *setOperationHeap; // used to sort nodes
        noDelIntSet *unionSet; // data structure used to calculate the set union

        // helper structures used in the set operations
        int *indices;
        int numIndices;

        // set operations
        bool setUnion(LmAoNode *update, int *combine, int size);

        bool setIntersection(LmAoNode *update, int *combine, int size);

        // operation of initial nodes
        bool setToSelfLM(LmAoNode *update);

        void ensureSizeCopy(LmAoNode *node, int size);

        bool reachable(int ei);

        void resizeIfSmallerNOCOPY(LmAoNode *node, int size);

        void printSetOpInput(LmAoNode *nodeToUpdate, const int *nodesToCombine, int size, string setOperation) const;


        // temporal SCC information
        void tarjan(int v);

        int maxdfs; // counter for dfs
        bool *U; // set of unvisited nodes
        vector<int> *S; // stack
        bool *containedS;
        int *dfsI;
        int *lowlink;

        int numSCCs = -1;
        int *nodeToSCC = nullptr;

        bool copyLms(LmAoNode *pNode, LmAoNode *pNode1);

    };

}

#endif //PANDAPIENGINE_LMCAUSAL_H
