/*
 * Heuristic.h
 *
 *  Created on: 22.09.2017
 *      Author: Daniel Höller
 */

#ifndef HEURISTICS_HHRC_H_
#define HEURISTICS_HHRC_H_

#include <set>
#include <forward_list>
#include "../../Model.h"
#include "../../intDataStructures/bucketSet.h"
#include "../../intDataStructures/bIntSet.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "hhRC2.h"
#include "hsAddFF.h"
#include "hsLmCut.h"
#include "hsFilter.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <list>


namespace progression {
    template<class ClassicalHeuristic>
    class hhRC : public Heuristic {
    private:
        /*set<int> s0set;
         set<int> gset;
         set<int> intSet;*/
        noDelIntSet gset;
        noDelIntSet intSet;
        bucketSet s0set;
        const bool correctTaskCount = true;
        const eEstimate estimate = estDISTANCE;

        void calcHtnGoalFacts(planStep *ps) {
            // call for subtasks
            for (int i = 0; i < ps->numSuccessors; i++) {
                if (ps->successorList[i]->goalFacts == nullptr) {
                    calcHtnGoalFacts(ps->successorList[i]);
                }
            }

            // calc goals for this step
            intSet.clear();
            for (int i = 0; i < ps->numSuccessors; i++) {
                for (int j = 0; j < ps->successorList[i]->numGoalFacts; j++) {
                    intSet.insert(ps->successorList[i]->goalFacts[j]);
                }
            }
            intSet.insert(ps->task);
            ps->numGoalFacts = intSet.getSize();
            ps->goalFacts = new int[ps->numGoalFacts];
            int k = 0;
            for (int t = intSet.getFirst(); t >= 0; t = intSet.getNext()) {
                ps->goalFacts[k++] = t;
            }
        }


/*
 * The original state bits are followed by one bit per action that is set iff
 * the action is reachable from the top. Then, there is one bit for each task
 * indicating that task has been reached bottom-up.
 */
        int t2tdr(int task) {
            return m->numStateBits + task;
        }

        int t2bur(int task) {
            return m->numStateBits + m->numActions + task;
        }

        void setHeuristicValue(searchNode *n) {
            // get facts holding in s0
            for (int i = 0; i < m->numStateBits; i++) {
                if (n->state[i]) {
                    s0set.insert(i);
                }
            }

            // generate goal
            for (int i = 0; i < m->gSize; i++) {
                gset.insert(m->gList[i]);
            }

            // add reachability facts and HTN-related goal
            for (int i = 0; i < n->numAbstract; i++) {
                if (n->unconstraintAbstract[i]->goalFacts == nullptr) {
                    calcHtnGoalFacts(n->unconstraintAbstract[i]);
                }

                // add reachability facts
                for (int j = 0; j < n->unconstraintAbstract[i]->numReachableT; j++) {
                    s0set.insert(t2tdr(n->unconstraintAbstract[i]->reachableT[j]));
                }

                // add goal facts
                for (int j = 0; j < n->unconstraintAbstract[i]->numGoalFacts; j++) {
                    gset.insert(t2bur(n->unconstraintAbstract[i]->goalFacts[j]));
                }
            }
            for (int i = 0; i < n->numPrimitive; i++) {
                if (n->unconstraintPrimitive[i]->goalFacts == nullptr) {
                    calcHtnGoalFacts(n->unconstraintPrimitive[i]);
                }

                // add reachability facts
                for (int j = 0; j < n->unconstraintPrimitive[i]->numReachableT; j++) {
                    s0set.insert(t2tdr(n->unconstraintPrimitive[i]->reachableT[j]));
                }

                // add goal facts
                for (int j = 0; j < n->unconstraintPrimitive[i]->numGoalFacts; j++) {
                    gset.insert(t2bur(n->unconstraintPrimitive[i]->goalFacts[j]));
                }
            }

            n->heuristicValue[index] = this->sasH->getHeuristicValue(s0set, gset);
            if (n->goalReachable) {
                n->goalReachable = (n->heuristicValue[index] != UNREACHABLE);
            }

            if (correctTaskCount) {
                if (n->goalReachable) {
                    set<int> done;
                    set<int> steps;
                    set<int> tasks;
                    forward_list<planStep *> todoList;

                    for (int i = 0; i < n->numAbstract; i++) {
                        todoList.push_front(n->unconstraintAbstract[i]);
                        done.insert(n->unconstraintAbstract[i]->id);
                    }
                    for (int i = 0; i < n->numPrimitive; i++) {
                        todoList.push_front(n->unconstraintPrimitive[i]);
                        done.insert(n->unconstraintPrimitive[i]->id);
                    }

                    while (!todoList.empty()) {
                        planStep *ps = todoList.front();
                        todoList.pop_front();
                        if (estimate == estCOSTS) {
                            if (ps->task < m->numActions) {
                                steps.insert(ps->id);
                                tasks.insert(ps->task);
                            }
                        } else {
                            steps.insert(ps->id);
                            tasks.insert(ps->task);
                        }
                        for (int i = 0; i < ps->numSuccessors; i++) {
                            planStep *subStep = ps->successorList[i];
                            if (done.find(subStep->id) == done.end()) {
                                todoList.push_front(subStep);
                                done.insert(subStep->id);
                            }
                        }
                    }
                    assert(steps.size() >= tasks.size());
                    n->heuristicValue += (steps.size() - tasks.size());
                }
            }

            s0set.clear();
            gset.clear();
        }

    public:
        ClassicalHeuristic *sasH;

        hhRC(Model *htn, Model* heuristicModel, int index, eEstimate estimate, bool correctTaskCount) : Heuristic(htn, index),
                                                                                 estimate(estimate),
                                                                                 correctTaskCount(correctTaskCount) {
            this->sasH = new ClassicalHeuristic(heuristicModel);
            m = htn;
            intSet.init(sasH->m->numStateBits);
            gset.init(sasH->m->numStateBits);
            s0set.init(sasH->m->numStateBits);
        }

        const Model *m;

        virtual ~hhRC() {
            delete this->sasH;
        }

        void setHeuristicValue(searchNode *n, searchNode *parent, int action) override {
            this->setHeuristicValue(n);
        }

        void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) override {
            this->setHeuristicValue(n);
        }
    };
} /* namespace progression */

#endif /* HEURISTICS_HHRC_H_ */
