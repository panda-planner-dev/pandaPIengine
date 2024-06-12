#include "mark.h"
#include "util.h"

MethodPrecMarker::MethodPrecMarker(Model *htn) {
    this->htn = htn;
    this->marked.assign(htn->numTasks, false);
    this->memorized.assign(htn->numTasks, false);
    this->props.resize(htn->numTasks);
    vector<bool> visited;
    visited.assign(htn->numMethods, false);
    for (int m = 0; m < htn->numMethods; m++) {
        this->dfs(m, visited);
    }
}

void MethodPrecMarker::dfs(int m, vector<bool> &visited) {
    if(visited[m]) return;
    visited[m] = true;
    int decomposedTask = htn->decomposedTask[m];
    vector<bool> isPrecTask(htn->numSubTasks[m], false);
    vector<bool> isActualTask(htn->numSubTasks[m], false);
    if (htn->numSubTasks[m] == 2) {
        if (htn->ordering[m][0] == 1 && htn->ordering[m][1] == 0) {
            swap(htn->subTasks[m][0], htn->subTasks[m][1]);
        }
    }
    for (int tIndex = 0; tIndex < htn->numSubTasks[m]; tIndex++) {
        int subTask = htn->subTasks[m][tIndex];
        string subTaskName = htn->taskNames[subTask];
        std::transform(subTaskName.begin(), subTaskName.end(), subTaskName.begin(), ::tolower);
        if (htn->isPrimitive[subTask]) {
            if (Util::isPrecondition(subTaskName)) {
                this->marked[subTask] = true;
                this->props[subTask].push_back(Util::extractPrecondition(subTask, htn));
                this->memorized[subTask] = true;
                isPrecTask[tIndex] = true;
            }
        } else {
            for (size_t mIndex = 0; mIndex < htn->numMethodsForTask[subTask]; mIndex++) {
                int nextM = htn->taskToMethods[subTask][mIndex];
                dfs(nextM, visited);
                if (this->marked[subTask]) isPrecTask[tIndex] = true;
            }
        }
    }
    if (htn->numSubTasks[m] == 1) {
        if (isPrecTask[0]) {
            this->marked[decomposedTask] = true;
            for (boost::dynamic_bitset<> *posProp : this->props[htn->subTasks[m][0]]) {
                this->props[decomposedTask].push_back(posProp);
                this->memorized[decomposedTask] = true;
            }
        }
    } else {
        if (isPrecTask[0] && isPrecTask[1]) this->marked[decomposedTask] = true;
    }
}

bool MethodPrecMarker::isMethodPrecSat(int t, int pos, PlanExecution *execution) {
    bool satisfied = false;
    // check each method precondition derived from this compound task
    if (this->memorized[t]) {
        vector<boost::dynamic_bitset<>*> posPropSets = this->props[t];
        boost::dynamic_bitset<> state = execution->getStateBits(pos);
        for (boost::dynamic_bitset<> *posProps : posPropSets) {
            if (posProps->is_subset_of(state)) {
                satisfied = true;
                break;
            }
        }
    } else {
        for (size_t mIndex = 0; mIndex < this->htn->numMethodsForTask[t]; mIndex++) {
            int m = this->htn->taskToMethods[t][mIndex];
            if (this->htn->numSubTasks[m] == 1) {
                int subTask = this->htn->subTasks[m][0];
                if (!this->marked[subTask]) continue;
                if (this->isMethodPrecSat(subTask, pos, execution)) {
                    satisfied = true;
                    break;
                }
            } else if (this->htn->numSubTasks[m] == 2) {
                bool bothSatisfied = true;
                if (!(this->marked[this->htn->subTasks[m][0]] && this->marked[this->htn->subTasks[m][1]]))
                    continue;
                for (size_t i = 0; i < 2; i++) {
                    int subTask = this->htn->subTasks[m][i];
                    if (!this->isMethodPrecSat(subTask, pos, execution)) {
                        bothSatisfied = false;
                        break;
                    }
                }
                if (bothSatisfied) {
                    satisfied = true;
                    break;
                }
            }
        }
    }
    return satisfied;
}