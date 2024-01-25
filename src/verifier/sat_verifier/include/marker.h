//
// Created by lst19 on 4/16/2023.
//

#ifndef PANDAPIENGINE_MARKER_H
#define PANDAPIENGINE_MARKER_H
#include "Model.h"

class TaskMarker{
public:
    TaskMarker(Model *htn) {
        this->htn = htn;
        this->nullable.assign(htn->numTasks, false);
        this->visited.assign(htn->numTasks, false);
        for (int task = 0; task < htn->numTasks; task++)
            this->dfs(task);
    }
    bool isNullable(int task) {return this->nullable[task];}

private:
    Model *htn;
    vector<bool> nullable;
    vector<bool> visited;
    void dfs(int task);
};
#endif //PANDAPIENGINE_MARKER_H
