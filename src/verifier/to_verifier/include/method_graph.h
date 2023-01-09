#include "Model.h"
#include "execution.h"
#include "graph.h"
#include "mark.h"

class MethodGraph : public DirectedGraph {
    public:
        MethodGraph(Model *htn, MethodPrecMarker *marker) : DirectedGraph(htn->numMethods) {
            // Construct a graph structure where each node is a method
            // and an edge m1 -> m2 indicates that the task decomposed
            // by m1 can be reached via m2 which is a *unit* production
            // by *unit* production, we account for the situation where
            // there is one subtask in m2 which is utimately decomposed
            // into a method precondition (or an empty task)
            for (int m = 0; m < htn->numMethods; m++) {
                if (htn->numSubTasks[m] == 1) {
                    // if m contains only one subtask, then
                    // for each method m' that can decompose this subtask
                    // we add an edge m' -> m 
                    int subTask = htn->subTasks[m][0];
                    if (htn->isPrimitive[subTask]) continue;
                    for (int mIndex = 0; mIndex < htn->numMethodsForTask[subTask]; mIndex++) {
                        int nextMethod = htn->taskToMethods[subTask][mIndex];
                        this->connect(nextMethod, m);
                        // this->edges[nextMethod].insert(m);
                    }
                } else if (htn->numSubTasks[m] == 2) {
                    // if m contains 2 subtasks and if one of them is
                    // decomposed to a method precondition, or is nullable
                    // then for each method m' that decomposes the other subtask
                    // we add an edge m' -> m
                    int firstTask = htn->subTasks[m][0];
                    int secondTask = htn->subTasks[m][1];
                    if (marker->isMarked(firstTask)) {
                        for (int mIndex = 0; mIndex < htn->numMethodsForTask[secondTask]; mIndex++) {
                            int nextMethod = htn->taskToMethods[secondTask][mIndex];
                            this->connect(nextMethod, m);
                            // this->edges[nextMethod].insert(m);
                        }
                    }
                    if (marker->isMarked(secondTask)) {
                        for (int mIndex = 0; mIndex < htn->numMethodsForTask[firstTask]; mIndex++) {
                            int nextMethod = htn->taskToMethods[firstTask][mIndex];
                            this->connect(nextMethod, m);                    
                            // this->edges[nextMethod].insert(m);
                        }
                    }
                } else continue;
            }
        }
};