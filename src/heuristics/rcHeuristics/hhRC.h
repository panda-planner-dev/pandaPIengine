/*
 * Heuristic.h
 *
 *  Created on: 22.09.2017
 *      Author: Daniel Höller
 */

#ifndef HEURISTICS_HHRC_H_
#define HEURISTICS_HEURISTIC_H_

#include <set>
#include <forward_list>
#include "../../Model.h"
#include "../../intDataStructures/bucketSet.h"
#include "../../intDataStructures/bIntSet.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "hsAddFF.h"
#include "hsLmCut.h"
#include "hsFilter.h"

namespace progression {

    template<class ClassicalHeuristic>
class hhRC : public Heuristic{
private:
	/*set<int> s0set;
	 set<int> gset;
	 set<int> intSet;*/
	noDelIntSet gset;
	noDelIntSet intSet;
	bucketSet s0set;

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

	void setHeuristicValue(searchNode *n);
public:
    ClassicalHeuristic* sasH;

    hhRC(Model* htn, int index) : Heuristic(htn, index){
        this->sasH = new ClassicalHeuristic;
        m = htn;
        intSet.init(sasH->m->numStateBits);
        gset.init(sasH->m->numStateBits);
        s0set.init(sasH->m->numStateBits);
    }

	const Model* m;
	virtual ~hhRC() {
        delete this->sasH;
    }

	void setHeuristicValue(searchNode *n, searchNode *parent, int action) override {

	}

	void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) override {

	}
};

} /* namespace progression */

#endif /* HEURISTICS_HHRC_H_ */
