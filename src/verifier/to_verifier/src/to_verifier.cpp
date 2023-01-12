#include "to_verifier.h"

void TOVerifier::updateTable(int start, int end) {
    unordered_set<int> validiated;
    unordered_set<int> visited;
    if (start == end) {
        // if we are looking at one particular
        // position in the plan
        int action = this->plan[start];
        this->table[start][end].insert(action);
        if (!this->invMapping->exists(action)) return;
        // prepare all possible unit production methods
        // which decompose some compound task to this action
        // again by 'unit", we already account for the situation
        // where one subtask in a method is decomposed into a
        // method precondition or is nullable
        unordered_set<int> possibleMethods = this->invMapping->getMethod(action);
        for (const auto &m : possibleMethods) {
            // do dfs on the construct graph in order to process all
            // unit production methods which can lead to the current one
            // dfsOnMethodGraph(m, start, end, isProcessed, hasAdded);
            dfs(m, start, end, visited, validiated);
        }
    } else {
        // if we are looking at a subsequence of the plan
        // then for each i with start <= i < end, we go through
        // all possible combinations of tasks contained by A[start, i]
        // and A[i + 1, end] respectively and check whether some of them
        // form a legal method (i.e., decomposition)
        for (int i = start; i < end; i++) {
            unordered_set<int> firstPossibleTasks = this->table[start][i];
            unordered_set<int> secondPossibleTasks = this->table[i + 1][end];
            for (const auto &firstTask : firstPossibleTasks) {
                for (const auto &secondTask : secondPossibleTasks) {
                    if (!this->invMapping->exists(firstTask, secondTask)) continue;
                    unordered_set<int> possibleMethods = this->invMapping->getMethod(firstTask, secondTask);
                    for (const auto &m : possibleMethods) {
                        this->table[start][end].insert(this->htn->decomposedTask[m]);
                        validiated.insert(m);
                        // again do dfs on the construct graph in order to process all
                        // unit production methods which can lead to the current one
                        // dfsOnMethodGraph(m, start, end, isProcessed, hasAdded);
                        dfs(m, start, end, visited, validiated);
                    }
                }
            }
        }
    }
}

void TOVerifier::dfs(int m, int start, int end, unordered_set<int> &visited, unordered_set<int> &validiated) {
    if (visited.count(m)) return;
    visited.insert(m);
    bool keepContinue = false;
    int decomposedTask = this->htn->decomposedTask[m];
    if (this->htn->numSubTasks[m] == 1) {
        table[start][end].insert(decomposedTask);
        keepContinue = true;
    } else {
        int firstTask = this->htn->subTasks[m][0];
        int secondTask = this->htn->subTasks[m][1];
        if (this->htn->ordering[m][0] == 1 && this->htn->ordering[m][1] == 0)
			swap(firstTask,secondTask);
        if (validiated.count(m)) keepContinue =true;
        if (this->precMarker->isMarked(firstTask)) {
            if (this->precMarker->isMethodPrecSat(firstTask, start, this->execution)) {
                table[start][end].insert(decomposedTask);
                keepContinue = true;
            }
        }
        if (this->precMarker->isMarked(secondTask)) {
            if (this->precMarker->isMethodPrecSat(secondTask, end + 1, this->execution)) {
                table[start][end].insert(decomposedTask);
                keepContinue = true;
            }
        }
    }
    if (!keepContinue) return;
    unordered_set<int>::iterator iter;
    for(iter = this->g->adjBegin(m); iter != this->g->adjEnd(m); iter++) {
        dfs(*iter, start, end, visited, validiated);
    } 
}