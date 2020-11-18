//
// Created by dh on 18.11.20.
//

#ifndef PANDAPIENGINE_ENFORCEDHILLCLIMBING_H
#define PANDAPIENGINE_ENFORCEDHILLCLIMBING_H

#include <queue>
#include <map>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <sys/time.h>
#include "../Model.h"
#include "QueueFringe.h"

using namespace std;

template<class H, class VL>
class EnforcedHillClimbing {

#ifdef TRACESOLUTION

    pair<string, int> extractSolutionFromSearchNode(Model *htn, searchNode *tnSol) {
        int sLength = 0;
        string sol = "";
        solutionStep *sost = tnSol->solution;
        bool done = sost == nullptr || sost->prev == nullptr;

        map<int, vector<pair<int, int>>> children;
        vector<pair<int, string>> decompositionStructure;

        int root = -1;

        while (!done) {
            sLength++;
            if (sost->method >= 0) {
                pair<int, string> application;
                application.first = sost->mySolutionStepInstanceNumber;
                application.second = htn->taskNames[sost->task] + " -> " + htn->methodNames[sost->method];
                decompositionStructure.push_back(application);
                if (sost->task == htn->initialTask) root = application.first;
            } else {
                sol = to_string(sost->mySolutionStepInstanceNumber) + " " +
                      htn->taskNames[sost->task] + "\n" + sol;
            }

            if (sost->mySolutionStepInstanceNumber != 0)
                children[sost->parentSolutionStepInstanceNumber].push_back(
                        make_pair(
                                sost->myPositionInParent,
                                sost->mySolutionStepInstanceNumber));

            done = sost->prev == nullptr;
            sost = sost->prev;
        }

        sol = "==>\n" + sol;
        sol = sol + "root " + to_string(root) + "\n";
        for (auto x : decompositionStructure) {
            sol += to_string(x.first) + " " + x.second;
            sort(children[x.first].begin(), children[x.first].end());
            for (auto[_, y] : children[x.first])
                sol += " " + to_string(y);
            sol += "\n";
        }

        sol += "<==";

        return make_pair(sol, sLength);
    }

#endif
public:
    void search(Model *htn, searchNode *tnI, int timeLimit, H *hF, VL *visitedList) {
        timeval tp;
        gettimeofday(&tp, NULL);
        long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        long currentT;
        bool reachedTimeLimit = false;

        this->m = htn;
        searchNode *tnSol = nullptr;
        bool solved = false;
        bool failed = false;

        cout << "Search Configuration" << endl;
        cout << "- Using JAIR 2019 progression algorithm" << endl;
        cout << "- Enforced Hill Climbing" << endl;

        QueueFringe fringe;
        tnI->heuristicValue = INT_MAX;

        visitedList->insertVisi(tnI);
        searchNode *nC = tnI;
        fringe.push(tnI);
        int numSearchNodes = 1;

        while (!solved) {
            if(failed) {
                break;
            }
            int h = nC->heuristicValue;
            cout << "New best heuristic value: " << h << endl;
            bool foundBetter = false;

            while (!foundBetter) { // start BFS
                if (fringe.empty()) {
                    failed = true;
                    break;
                }
                searchNode *n = fringe.top();
                assert(n != nullptr);
                fringe.pop();

                if (n->numAbstract == 0) {
                    for (int i = 0; i < n->numPrimitive; i++) {
                        if (!htn->isApplicable(n, n->unconstraintPrimitive[i]->task))
                            continue;
                        searchNode *n2 = htn->apply(n, i);
                        numSearchNodes++;
                        if (!n2->goalReachable || !visitedList->insertVisi(n2)) { // progression has detected unsol
                            delete n2;
                            continue;
                        }
                        hF->setHeuristicValue(n2, n, n->unconstraintPrimitive[i]->task);
                        assert(n2->goalReachable || (!htn->isGoal(n2))); // otherwise the heuristic is not save

                        if (n2->goalReachable) {
                            if (htn->isGoal(n2)) {
                                tnSol = n2;
                                solved = true;
                                break;
                            } else if (n2->heuristicValue < h) {
                                nC = n2;
                                while (!fringe.empty()) {
                                    searchNode *n3 = fringe.top();
                                    fringe.pop();
                                    delete n3;
                                }
                                foundBetter = true;
                                break;
                            } else {
                                fringe.push(n2);
                            }
                        } else {
                            delete n2;
                        }
                    }
                    if (solved) break;
                } else if (n->numAbstract > 0) {
                    int decomposedStep = rand() % n->numAbstract;
                    int task = n->unconstraintAbstract[decomposedStep]->task;
                    for (int i = 0; i < htn->numMethodsForTask[task]; i++) {
                        int method = htn->taskToMethods[task][i];
                        searchNode *n2 = htn->decompose(n, decomposedStep, method);
                        numSearchNodes++;
                        if (!n2->goalReachable || !visitedList->insertVisi(n2)) { // decomposition has detected unsol
                            delete n2;
                            continue; // with next method
                        }
                        hF->setHeuristicValue(n2, n, decomposedStep, method);
                        assert(n2->goalReachable || (!htn->isGoal(n2))); // otherwise the heuristic is not save

                        if (n2->goalReachable) {
                            if (htn->isGoal(n2)) {
                                tnSol = n2;
                                solved = true;
                                break;
                            } else if (n2->heuristicValue < h) {
                                nC = n2;
                                while (!fringe.empty()) {
                                    searchNode *n3 = fringe.top();
                                    fringe.pop();
                                    delete n3;
                                }
                                foundBetter = true;
                                break;
                            } else {
                                fringe.push(n2);
                            }
                        } else {
                            delete n2;
                        }
                    }
                }
            }
        }
        gettimeofday(&tp, NULL);
        currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        cout << "Search Results" << endl;
        cout << "- Search time " << double(currentT - startT) / 1000 << " seconds" << endl;
        cout << "- Generated " << (numSearchNodes + htn->numOneModActions + htn->numOneModMethods + htn->numEffLessProg)
             << " search nodes" << endl;
        cout << "  Calculated heuristic for " << numSearchNodes << " nodes" << endl;
        cout << "- Generated " << int(double(numSearchNodes) / (currentT - startT) * 1000) << " nodes per second"
             << endl;
        if (tnSol != nullptr) {
#ifdef TRACESOLUTION
            auto[sol, sLength] = extractSolutionFromSearchNode(htn, tnSol);
#else
            auto [sol,sLength] = printTraceOfSearchNode(htn,tnSol);
#endif
            cout << "- Status: Solved" << endl;
            cout << "- Found solution of length " << sLength << endl;
            cout << "- Total costs of actions: " << tnSol->actionCosts << endl
                 << endl;
            //cout << sol << endl;
        } else if (reachedTimeLimit) {
            cout << "- Status: Timeout" << endl;
        } else {
            cout << "- Status: Proven unsolvable" << endl;
        }

#ifndef NDEBUG
        cout << "Deleting elements in fringe..." << endl;
        while (!fringe.empty()) {
            searchNode *n = fringe.top();
            fringe.pop();
            delete n;
        }
        delete tnSol;
        delete hF;
#endif
    }

private:
    Model* m;
};


#endif //PANDAPIENGINE_ENFORCEDHILLCLIMBING_H
