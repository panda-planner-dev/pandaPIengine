#ifndef _mapping_inc_h_
#define _mapping_inc_h_

#include "Model.h"
#include "mark.h"
#include <unordered_map>

class InverseMapping {
    public:
        InverseMapping(Model *htn, MethodPrecMarker *marker) {
            // construct inverse mapping from tasks to methods
            // "single" map a single task to the method
            // that decomposes some compound task to it
            // for this, we should also account for methods
            // where one subtask can eventually be decomposed
            // into method precondition or is nullable, i.e.,
            // we regard such methods as unit methods as well

            // "couple" maps two tasks to methods which
            // can decompose some compound task to them
            for (int m = 0; m < htn->numMethods; m++) {
                if (htn->numSubTasks[m] == 1) {
                    int subTask = htn->subTasks[m][0];
                    this->single[subTask].insert(m);
                } else if (htn->numSubTasks[m] == 2) {
                    int firstTask = htn->subTasks[m][0];
                    int secondTask = htn->subTasks[m][1];
                    if (marker->isMarked(firstTask)) {
                        this->single[secondTask].insert(m);
                    }
                    if (marker->isMarked(secondTask)) {
                        this->single[firstTask].insert(m);
                    }
                    this->couple[firstTask][secondTask].insert(m);
                } else continue;
            }
        }
        unordered_set<int> getMethod(int t) {return single[t];}
        unordered_set<int> getMethod(int first, int second) {return couple[first][second];}
        bool exists(int t) {return this->single.count(t);}
        bool exists(int first, int second) {
            return this->couple.count(first) && this->couple[first].count(second);
        }

    private:
        unordered_map<int, unordered_set<int>> single;
        unordered_map<int,unordered_map<int,unordered_set<int>>> couple;
};

#endif