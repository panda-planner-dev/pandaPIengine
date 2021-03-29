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

void hhRC::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
	this->setHeuristicValue(n);
}

void hhRC::setHeuristicValue(searchNode *n, searchNode *parent, int absTask,
		int method) {
	this->setHeuristicValue(n);
}

void hhRC::setHeuristicValue(searchNode *n) {
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

	s0set.clear();
	gset.clear();
}

#endif
} /* namespace progression */
