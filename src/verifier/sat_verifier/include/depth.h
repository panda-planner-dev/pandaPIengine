//
// Created by lst19 on 4/16/2023.
//

#ifndef PANDAPIENGINE_DEPTH_H
#define PANDAPIENGINE_DEPTH_H
#include "Model.h"
#include "marker.h"
#include "distribution.h"
#include "fstream"
#include <string.h>
class Depth {
public:
    Depth(Model *htn, int length);
    int get(int task, int length) {return this->depth[task][length];}
    int get() {return this->depth[this->htn->initialTask][this->maxLength];}
private:
    Model *htn;
    int maxLength;
    vector<Distributions*> distributions;
    vector<vector<int>> depth;
    void depthPerSCC(int length, int scc);
    void update(int length, int scc, bool allowEmptiness);
};
#endif //PANDAPIENGINE_DEPTH_H
