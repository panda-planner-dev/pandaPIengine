/*
 * planninggGraph.cpp
 *
 *  Created on: 26.10.2018
 *      Author: Daniel Höller
 */

#include "planningGraph.h"

#include <cassert>
#include <cstring>

namespace progression {

planningGraph::planningGraph(Model* sas) {
	m = sas;
	hValPropInit = new int[m->numStateBits];
	for (int i = 0; i < m->numStateBits; i++) {
		hValPropInit[i] = UNREACHABLE;
	}
	queue = new IntPairHeap<int>(m->numStateBits * 2);
	numSatPrecs = new int[m->numActions];
	hValOp = new int[m->numActions];
	hValProp = new int[m->numStateBits];
	markedFs.init(m->numStateBits);
	markedOps.init(m->numActions);
	needToMark.init(m->numStateBits);
	reachableTasksSet.init(m->numTasks);

	stack = new IntStack();
	stack->init(m->numTasks);

	subtasks = new int[m->numMethods];
	reachableMethodsSet.init(m->numMethods);
	usefulFactSet.init(m->numStateBits);
}

planningGraph::~planningGraph() {
	delete[] hValPropInit;
	delete[] numSatPrecs;
	delete[] hValOp;
	delete[] hValProp;
	delete queue;
	delete stack;
}

void planningGraph::calcReachability(vector<bool>& s,
		noDelIntSet& reachableIn) {
	reachableTasksSet.clear();
	memcpy(numSatPrecs, m->numPrecs, sizeof(int) * m->numActions);
	memcpy(hValOp, m->actionCosts, sizeof(int) * m->numActions);
	memcpy(hValProp, hValPropInit, sizeof(int) * m->numStateBits);

	usefulFactSet.clear();
	for (int i = 0; i < m->gSize; i++) {
		if (!s[m->gList[i]])
			usefulFactSet.insert(m->gList[i]);
	}

	queue->clear();
	for (unsigned int f = 0; f < s.size(); f++) {
		if (s[f]) {
			queue->add(0, f);
			hValProp[f] = 0;
		}
	}

	for (int i = 0; i < m->numPrecLessActions; i++) {
		int ac = m->precLessActions[i];
        if (!reachableIn.get(ac))
            continue;
		reachableTasksSet.insert(ac);
		for (int iAdd = 0; iAdd < m->numAdds[ac]; iAdd++) {
			int fAdd = m->addLists[ac][iAdd];
			hValProp[fAdd] = m->actionCosts[ac];
			queue->add(hValProp[fAdd], fAdd);
		}
	}

	while (!queue->isEmpty()) {
		int pVal = queue->topKey();
		int prop = queue->topVal();
		queue->pop();
		if (hValProp[prop] < pVal)
		    continue;
		for (int iOp = 0; iOp < m->precToActionSize[prop]; iOp++) {
			int op = m->precToAction[prop][iOp];
			hValOp[op] += pVal;
			if ((--numSatPrecs[op] == 0) && (reachableIn.get(op))) {
				reachableTasksSet.insert(op);
				for (int i = 0; i < m->numPrecs[op]; i++) {
					if (!s[m->precLists[op][i]]) {
						usefulFactSet.insert(m->precLists[op][i]);
					}
				}
				for (int iF = 0; iF < m->numAdds[op]; iF++) {
					int f = m->addLists[op][iF];
					if (hValOp[op] < hValProp[f]) {
						hValProp[f] = hValOp[op];
						queue->add(hValProp[f], f);
					}
				}
			}
		}
	}
	memcpy(subtasks, m->numDistinctSTs, sizeof(int) * m->numMethods);
	stack->clear();
	for (int x = reachableTasksSet.getFirst(); x >= 0;
			x = reachableTasksSet.getNext()) {
		stack->push(x);
	}
	reachableMethodsSet.clear();
	while (!stack->isEmpty()) {
		int t = stack->pop();
		for (int i = 0; i < m->stToMethodNum[t]; i++) {
			int method = m->stToMethod[t][i];
			if ((--subtasks[method] == 0) && (reachableIn.get(m->decomposedTask[method]))) {
				reachableMethodsSet.insert(method);
				if (!reachableTasksSet.get(m->decomposedTask[method])) {
					reachableTasksSet.insert(m->decomposedTask[method]);
					stack->push(m->decomposedTask[method]);
				}
			}
		}
	}
}

bool planningGraph::factReachable(int i) {
	return hValProp[i] != UNREACHABLE;
}

bool planningGraph::taskReachable(int i) {
	return reachableTasksSet.get(i);
}

bool planningGraph::methodReachable(int i) {
	return reachableMethodsSet.get(i);
}

} /* namespace progression */
