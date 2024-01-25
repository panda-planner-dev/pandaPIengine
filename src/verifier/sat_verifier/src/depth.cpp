//
// Created by lst19 on 4/16/2023.
//
#include <cassert>
#include "depth.h"
#include "util.h"

Depth::Depth(Model *htn, int length) {
    this->maxLength = length;
    this->htn = htn;
    this->htn->calcSCCs();
    this->htn->calcSCCGraph();
    this->htn->analyseSCCcyclicity();
    this->depth.resize(this->htn->numTasks);
    for (int t = 0; t < this->htn->numTasks; t++)
        this->depth[t].assign(length + 1, -1);
    this->distributions.resize(this->htn->numMethods);
    for (int m = 0; m < this->htn->numMethods; m++)
        this->distributions[m] = new Distributions(
                m, length, this->htn);
    for (int i = 0; i < this->htn->numSCCs; i++) {
        int scc = htn->sccTopOrder[i];
        this->depthPerSCC(
                length, scc);
    }
}

void Depth::depthPerSCC(
        int length,
        int scc) {
    int size = htn->sccSize[scc];
    for (int i = 1; i <= length; i++) {
        this->update(length, scc, false);
        for (int j = 1; j <= size - 1; j++)
            this->update(length, scc, true);
    }
}

void Depth::update(
        int length,
        int scc,
        bool allowEmptiness) {
    for (int l = 0; l <= length; l++) {
        int size = this->htn->sccSize[scc];
        for (int tInd = 0; tInd < size; tInd++) {
            int task = this->htn->sccToTasks[scc][tInd];
            if (this->htn->isPrimitive[task]) {
                assert(size == 1);
                string name = this->htn->taskNames[task];
                if (Util::isPrecondition(name))
                    this->depth[task][0] = 0;
                else
                    this->depth[task][1] = 0;
                continue;
            }
            int numMethods = this->htn->numMethodsForTask[task];
            for (int mInd = 0; mInd < numMethods; mInd++) {
                int m = this->htn->taskToMethods[task][mInd];
                this->distributions[m]->update(
                        l, this->depth, allowEmptiness);
                if (this->distributions[m]->isDistributable(l))
                    this->depth[task][l] = max(
                            this->depth[task][l],
                            1 + this->distributions[m]->maxDepth(l));
            }
        }
    }
}