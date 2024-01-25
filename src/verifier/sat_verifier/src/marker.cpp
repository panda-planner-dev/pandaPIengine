//
// Created by lst19 on 4/16/2023.
//
#include "marker.h"
#include "util.h"

void TaskMarker::dfs(int task) {
    if (this->visited[task])
        return;
    this->visited[task] = true;
    if (this->htn->isPrimitive[task]) {
        string taskName = this->htn->taskNames[task];
        if (Util::isPrecondition(taskName))
            this->nullable[task];
    } else {
        int numMethods = this->htn->numMethodsForTask[task];
        for (int mInd = 0; mInd < numMethods; mInd++) {
            int method = this->htn->taskToMethods[task][mInd];
            int numSubTasks = this->htn->numSubTasks[method];
            bool nullableTask = true;
            for (int tInd = 0; tInd < numSubTasks; tInd++) {
                int subTask = this->htn->subTasks[method][tInd];
                this->dfs(subTask);
                if (!this->nullable[subTask]) {
                    nullableTask = false;
                    break;
                }
            }
            if (nullableTask) {
                this->nullable[task] = true;
                break;
            }
        }
    }
}
