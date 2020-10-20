/*
 * HtnAddFF.cpp
 *
 *  Created on: 22.09.2017
 *      Author: Daniel Höller
 */

#include "hhRC.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <list>

namespace progression {

#if (HEURISTIC == RCFF || HEURISTIC == RCADD)
hhRC::hhRC(Model* htn, hsAddFF* sasH) {
#elif (HEURISTIC == RCLMC)
hhRC::hhRC(Model* htn, hsLmCut* sasH) {
#else
	hhRC::hhRC(Model* htn, hsFilter* sasH) {
#endif

#ifdef RCLMOPTIMAL
	cout << "HTN model" << endl;
	for (int i = 0; i < htn->numActions; i++) {
		std::string prefix("SHOP_");
		if (!sasH->m->taskNames[i].compare(0, prefix.size(), prefix)) {
			sasH->m->actionCosts[i] = 0;
		}
		cout << sasH->m->taskNames[i] << " " << sasH->m->actionCosts[i] << endl;
	}
	cout << endl << "Heuristic model" << endl;
	for (int i = 0; i < sasH->m->numActions; i++) {
		std::string methodMarker("@");
		if (sasH->m->taskNames[i].find(methodMarker) != std::string::npos) {
			sasH->m->actionCosts[i] = 0;
		}
		std::string prefix("SHOP_");
		if (!sasH->m->taskNames[i].compare(0, prefix.size(), prefix)) {
			sasH->m->actionCosts[i] = 0;
		}

		cout << sasH->m->taskNames[i] << " " << sasH->m->actionCosts[i] << endl;
	}
#endif
	this->sasH = sasH;
	m = htn;
	intSet.init(sasH->m->numStateBits);
	gset.init(sasH->m->numStateBits);
	s0set.init(sasH->m->numStateBits);

/*
#ifdef CORRECTTASKCOUNT
	htn->calcMinimalImpliedX();
#endif
*/
}

hhRC::~hhRC() {
	delete this->sasH;
}

#ifdef RCHEURISTIC
void hhRC::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
	this->setHeuristicValue(n);
}

void hhRC::setHeuristicValue(searchNode *n, searchNode *parent, int absTask,
		int method) {
	this->setHeuristicValue(n);
}

void hhRC::setHeuristicValue(searchNode *n) {
	// get facts holding in s0
#if STATEREP == SRCOPY
	for (int i = 0; i < m->numStateBits; i++) {
		int pos1 = i / 64;
		int pos2 = i % 64;
		if (n->state[pos1] & (uint64_t(1) << pos2)) {
			s0set.insert(i);
		}
	}
#elif STATEREP == SRCALC1
	assert(s0set.isEmpty());
	for (int i = 0; i < m->s0Size; i++) {
		s0set.insert(m->s0List[i]);
	}
	if (n->solution != nullptr)
	collectState(n->solution, s0set);
#elif STATEREP == SRCALC2
	assert(s0set.isEmpty());
	intSet.clear();
	collectState(n->solution, s0set, intSet);
	for (int i = 0; i < m->s0Size; i++) {
		if (!intSet.get(m->s0List[i]))
		s0set.insert(m->s0List[i]);
	}

	intSet.clear();
#elif STATEREP == SRLIST
	for(int i =0; i< n->stateSize;i++) {
		s0set.insert(n->state[i]);
	}
#endif

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

	n->heuristicValue = this->sasH->getHeuristicValue(s0set, gset);
	n->goalReachable = (n->heuristicValue != UNREACHABLE);

#ifdef CORRECTTASKCOUNT
	if (n->goalReachable) {
		set<int> done;
		set<int> steps;
		set<int> tasks;
		forward_list<planStep*> todoList;

		for (int i = 0; i < n->numAbstract; i++) {
			todoList.push_front(n->unconstraintAbstract[i]);
			done.insert(n->unconstraintAbstract[i]->id);
		}
		for (int i = 0; i < n->numPrimitive; i++) {
			todoList.push_front(n->unconstraintPrimitive[i]);
			done.insert(n->unconstraintPrimitive[i]->id);
		}

		while (!todoList.empty()) {
			planStep* ps = todoList.front();
			todoList.pop_front();
#ifdef ONLYCOUNTACTIONS
			if (ps->task < m->numActions) {
				steps.insert(ps->id);
				tasks.insert(ps->task);
			}
#else
			steps.insert(ps->id);
			tasks.insert(ps->task);
#endif
			for (int i = 0; i < ps->numSuccessors; i++) {
				planStep* subStep = ps->successorList[i];
				if (done.find(subStep->id) == done.end()) {
					todoList.push_front(subStep);
					done.insert(subStep->id);
				}
			}
		}
		assert(steps.size() >= tasks.size());
		n->heuristicValue += (steps.size() - tasks.size());
	}
#endif

/* this is the new method for the calculation
#ifdef CORRECTTASKCOUNT
	if (n->goalReachable) {
//		cout << "--- START ---" << endl;
		set<int> done;
		set<int> tasks;
		forward_list<planStep*> todoList;

		for (int i = 0; i < n->numAbstract; i++) {
			todoList.push_front(n->unconstraintAbstract[i]);
			done.insert(n->unconstraintAbstract[i]->id);
//			cout << "pushed " << n->unconstraintAbstract[i]->id << endl;
		}
		for (int i = 0; i < n->numPrimitive; i++) {
			todoList.push_front(n->unconstraintPrimitive[i]);
			done.insert(n->unconstraintPrimitive[i]->id);
//			cout << "pushed " << n->unconstraintPrimitive[i]->id << endl;
		}

		while (!todoList.empty()) {
			planStep* ps = todoList.front();
			todoList.pop_front();
//			cout << "popped " << ps->id << endl;

			if (tasks.find(ps->task) == tasks.end()) {
//				cout << "found first " << ps->task << endl;
				tasks.insert(ps->task); // this is the first one found
			} else { // found more than once -> need to correct heuristic
#ifdef RCCOUNTCOSTS
				n->heuristicValue++;
#else
				n->heuristicValue += m->minImpliedDistance[ps->task];
//				cout << "found next " << ps->task << endl;
//				cout << "added " << m->minImpliedDistance[ps->task] << endl;
#endif
			}

			for (int i = 0; i < ps->numSuccessors; i++) {
				planStep* subStep = ps->successorList[i];
				if (done.find(subStep->id) == done.end()) {
					todoList.push_front(subStep);
//					cout << "pushed " << subStep->id << endl;
					done.insert(subStep->id);
				}
			}
		}
	}
#endif
*/
	s0set.clear();
	gset.clear();
}

#if STATEREP == SRCALC1
void hhRC::collectState(solutionStep* sol, bucketSet& s0set) {
	if (sol->prev != nullptr) {
		collectState(sol->prev, s0set);
	}
	assert(sol->task < m->numTasks);
	assert(sol->task >= 0);
	if (m->isPrimitive[sol->task]) {
		for (int i = 0; i < m->numDels[sol->task]; i++) {
			s0set.erase(m->delLists[sol->task][i]);
		}
		for (int i = 0; i < m->numAdds[sol->task]; i++) {
			s0set.insert(m->addLists[sol->task][i]);
		}
	}
}
#elif STATEREP == SRCALC2
void hhRC::collectState(solutionStep* sol, noDelIntSet& adds,
		noDelIntSet& dels) {
	if (m->isPrimitive[sol->task]) {
		for (int i = 0; i < m->numAdds[sol->task]; i++) {
			if (!dels.get(m->addLists[sol->task][i]))
			adds.insert(m->addLists[sol->task][i]);
		}
		for (int i = 0; i < m->numDels[sol->task]; i++) {
			if (!adds.get(m->delLists[sol->task][i]))
			dels.insert(m->delLists[sol->task][i]);
		}
	}
	if (sol->prev != nullptr) {
		collectState(sol->prev, adds, dels);
	}
}
#endif

void hhRC::calcHtnGoalFacts(planStep *ps) {
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
int hhRC::t2tdr(int task) {
	return m->numStateBits + task;
}

int hhRC::t2bur(int task) {
	return m->numStateBits + m->numActions + task;
}

#endif
} /* namespace progression */
