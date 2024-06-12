//
// Created by u6162630 on 4/17/23.
//

#ifndef PANDAPIENGINE_DISTRIBUTION_H
#define PANDAPIENGINE_DISTRIBUTION_H

#include <climits>
#include "vector"
#include "Model.h"

using namespace std;
class LengthDistributions {
public:
    LengthDistributions(int total, int numCols) {
        this->numCols = numCols;
        this->distribution = new int*[INT_MAX];
        this->numRows = this->generate(total, 0, 0);
        this->row = 0;
    }
    int* next() {
        if (this->row >= this->numRows)
            return nullptr;
        int r = this->row;
        this->row++;
        return this->distribution[r];
    }
    void reset() {this->row = 0;}
    int numDistributions() {return this->numRows;}
private:
    int** distribution;
    int row;
    int numCols;
    int numRows;
    int generate(int total, int row, int col);
};

class Distributions {
public:
    Distributions(int method, int length, Model *htn) {
        int numTasks = htn->numSubTasks[method];
        this->valid.resize(numTasks);
        for (int i = 0; i < numTasks; i++)
            this->valid[i].assign(length + 1, false);
        this->depth.resize(numTasks);
        for (int i = 0; i < numTasks; i++)
            this->depth[i].assign(length + 1, -1);
        this->method = method;
        this->htn = htn;
    }
    void update(int l, vector<vector<int>> &K, bool allowEmptiness) {
        int numTasks = this->htn->numSubTasks[this->method];
        int rootTask = this->htn->decomposedTask[this->method];
        int lastTask = this->htn->subTasks[this->method][numTasks - 1];
        int subtaskSCC = this->htn->taskToSCC[lastTask];
        int rootSCC = this->htn->taskToSCC[rootTask];
        if ((subtaskSCC == rootSCC) == allowEmptiness) {
            this->valid[numTasks - 1][l] = (K[lastTask][l] != -1);
            this->depth[numTasks - 1][l] = max(
                    this->depth[numTasks - 1][l],
                    K[lastTask][l]);
        }
        for (int tInd = numTasks - 2; tInd >= 0; tInd--) {
            int task = this->htn->subTasks[this->method][tInd];
            subtaskSCC = this->htn->taskToSCC[task];
            if ((subtaskSCC == rootSCC) == allowEmptiness) {
                this->valid[tInd][l] = (K[task][l] != -1) &&
                                       this->valid[tInd + 1][0];
                this->depth[tInd][l] = max(
                        this->depth[tInd][l],
                        K[task][l]);
            }
            if (allowEmptiness) continue;
            for (int assignment = 0; assignment < l; assignment++) {
                int depthForTask = K[task][assignment];
                if (depthForTask == -1) continue;
                int remain = l - assignment;
                if (this->valid[tInd + 1][remain]) {
                    this->valid[tInd][l] = true;
                    int depthForSubtasks = this->depth[tInd + 1][remain];
                    this->depth[tInd][l] = max(
                            this->depth[tInd][l],
                            max(depthForTask, depthForSubtasks));
                }
            }
        }
    }
    bool isDistributable(int l) {return this->valid[0][l];}
    int maxDepth(int l) {return this->depth[0][l];}
private:
    Model *htn;
    int method;
    vector<vector<bool>> valid;
    vector<vector<int>> depth;
};
#endif //PANDAPIENGINE_DISTRIBUTION_H
