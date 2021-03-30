/*
 * hsLmCut.cpp
 *
 *  Created on: 13.12.2018
 *      Author: Daniel Höller
 */

#include "hsLmCut.h"
#include <cstring>

namespace progression {

hsLmCut::hsLmCut(Model* sas) {
	// init hMax-Heuristic
	assert(!sas->isHtnModel);
	m = sas;
	hValInit = new int[m->numStateBits];
	for (int i = 0; i < m->numStateBits; i++) {
		hValInit[i] = UNREACHABLE;
	}
	heap = new IntPairHeap<int>(m->numStateBits * 2);
	unsatPrecs = new int[m->numActions];
	hVal = new int[m->numStateBits];

	maxPrecInit = new int[m->numActions];
	for (int i = 0; i < m->numActions; i++)
		maxPrecInit[i] = UNREACHABLE;

	// init hLmCut
	costs = new int[m->numActions];
	goalZone = new noDelIntSet();
	goalZone->init(m->numStateBits);
	cut = new bucketSet();
	cut->init(m->numActions);
	precsOfCutNodes = new bucketSet();
	precsOfCutNodes->init(m->numStateBits);
	stack.init(m->numActions * 2);

	// init reverse mapping
	numAddToTask = new int[m->numStateBits];
	for (int i = 0; i < m->numStateBits; i++)
		numAddToTask[i] = 0;
	for (int i = 0; i < m->numActions; i++) {
		for (int j = 0; j < m->numAdds[i]; j++) {
			int f = m->addLists[i][j];
			numAddToTask[f]++;
		}
	}
	addToTask = new int*[m->numStateBits];
	int *temp = new int[m->numStateBits];
	for (int i = 0; i < m->numStateBits; i++) {
		addToTask[i] = new int[numAddToTask[i]];
		temp[i] = 0;
	}
	for (int iOp = 0; iOp < m->numActions; iOp++) {
		for (int iF = 0; iF < m->numAdds[iOp]; iF++) {
			int f = m->addLists[iOp][iF];
			addToTask[f][temp[f]] = iOp;
			temp[f]++;
			if (temp[f] > 1) { // otherwise, an action has the same effect twice
				assert(addToTask[f][temp[f] - 2] < addToTask[f][temp[f] - 1]);
			}
		}
	}
	delete[] temp;
	remove = new noDelIntSet();
	remove->init(m->numStateBits);
	maxPrec = new int[m->numActions];
	visited = new noDelIntSet();
	visited->init(m->numStateBits);
}

hsLmCut::~hsLmCut() {
	delete[] hValInit;
	delete[] unsatPrecs;
	delete[] hVal;
	delete[] costs;
	delete heap;
	delete goalZone;
	delete cut;
	delete precsOfCutNodes;
	delete[] maxPrecInit;
	delete[] numAddToTask;
	for (int i = 0; i < m->numStateBits; i++)
		delete[] addToTask[i];
	delete[] addToTask;
	delete remove;
	delete[] maxPrec;
	delete visited;

}

int hsLmCut::getHeuristicValue(bucketSet& s, noDelIntSet& g) {
	int hLmCut = 0;

	// clean up stored cuts

	if(storeCuts) {
		for(LMCutLandmark* cut : *cuts) {
			delete cut;
		}
		cuts->clear();
	}

	memcpy(costs, m->actionCosts, sizeof(int) * m->numActions);

	int hMax = getHMax(s, g);
	if ((hMax == 0) || (hMax == UNREACHABLE))
		return hMax;
	//cout << endl << "start" << endl;

	// Ausgabe
	/*
	bool factsUsed[m->numStateBits];
	for (int i = 0; i < m->numStateBits; i++) {
	    factsUsed[i] = s.get(i);
	}
	for (int i = 0; i < this->m->numActions; i++) {
        if (maxPrec[i] != UNREACHABLE) {
            factsUsed[i] = true;
            for(int j =0 ; j < m->numAdds[i]; j++) {
                int f = m->addLists[i][j];
                factsUsed[f] = true;
            }
        }
	}
    for (int i = 0; i < m->numStateBits; i++) {
        if(factsUsed[i]) {
            cout << "node" << i << " "
        }
    }
*/

	while (hMax > 0) {
		goalZone->clear();
		cut->clear();
		precsOfCutNodes->clear();

		calcGoalZone(goalZone, cut, precsOfCutNodes);
		assert(cut->getSize() > 0);

		// check forward-reachability
		forwardReachabilityDFS(s, cut, goalZone, precsOfCutNodes);
		assert(cut->getSize() > 0);

		// calculate costs
		int minCosts = INT_MAX;

		LMCutLandmark* currendCut = nullptr;
		int ci = 0;
		if (storeCuts) {
            currendCut = new LMCutLandmark(cut->getSize());
			cuts->push_back(currendCut);
		}
		for (int cutted = cut->getFirst(); cutted >= 0; cutted =
				cut->getNext()) {
			if (minCosts > costs[cutted])
				minCosts = costs[cutted];
			if (storeCuts)
                currendCut->lm[ci++] = cutted;
		}
		assert(minCosts > 0);
		hLmCut += minCosts;

		// update costs
		//cout << "cut" << endl;
		for (int op = cut->getFirst(); op >= 0; op = cut->getNext()) {
			//cout << "- [" << op << "] " << m->taskNames[op] << endl;
			costs[op] -= minCosts;
			assert(costs[op] >= 0);
			//assert(allPrecsTrue(op));
		}
#ifdef LMCINCHMAX
		hMax = updateHMax(g, cut);
#else
		hMax = getHMax(s, g);
#endif
	}
	//cout << "final lmc " << hLmCut << endl;
	return hLmCut;
}

void hsLmCut::calcGoalZone(noDelIntSet* goalZone, bucketSet* cut,
		bucketSet* precsOfCutNodes) {
	stack.clear();
	stack.push(maxPrecG);
	while (!stack.isEmpty()) {
		int fact = stack.pop();

		for (int i = 0; i < numAddToTask[fact]; i++) {
			int producer = addToTask[fact][i];

			if (unsatPrecs[producer] > 0) // not reachable
				continue;

			int singlePrec = maxPrec[producer];
			if (goalZone->get(singlePrec))
				continue;

			if (costs[producer] == 0) {
				goalZone->insert(singlePrec);
				precsOfCutNodes->erase(singlePrec);
				stack.push(singlePrec);
			} else {
				cut->insert(producer);
				precsOfCutNodes->insert(singlePrec);
			}
		}
	}
}

void hsLmCut::forwardReachabilityDFS(bucketSet& s0, bucketSet* cut,
		noDelIntSet* goalZone, bucketSet* testReachability) {
	stack.clear();
	remove->clear();

	for (int f = testReachability->getFirst(); f >= 0;
			f = testReachability->getNext()) {
		if (s0.get(f))
			continue;
		visited->clear();
		bool reachedS0 = false;
		stack.clear();
		stack.push(f);
		visited->insert(f);
		while (!stack.isEmpty()) { // reachabilityLoop
			int pred = stack.pop();
			for (int i = 0; i < numAddToTask[pred]; i++) {
				int op = addToTask[pred][i];
				if (unsatPrecs[op] > 0)
					continue;
				if (goalZone->get(maxPrec[op]))
					continue;
				if ((m->numPrecs[op] == 0) || (s0.get(maxPrec[op]))) { // reached s0
					reachedS0 = true;
					break;
				} else if (!visited->get(maxPrec[op])) {
					visited->insert(maxPrec[op]);
					stack.push(maxPrec[op]);
				}
			}
			if (reachedS0)
				break;
		}
		if (!reachedS0)
			remove->insert(f);
	}
	for (int op = cut->getFirst(); op >= 0; op = cut->getNext()) {
		if (remove->get(maxPrec[op]))
			cut->erase(op);
	}
}

int hsLmCut::getHMax(bucketSet& s, noDelIntSet& g) {
	if (g.getSize() == 0)
		return 0;

	memcpy(unsatPrecs, m->numPrecs, sizeof(int) * m->numActions);
	memcpy(maxPrec, maxPrecInit, sizeof(int) * m->numActions);
	memcpy(hVal, hValInit, sizeof(int) * m->numStateBits);
	maxPrecG = -1;

	heap->clear();
	for (int f = s.getFirst(); f >= 0; f = s.getNext()) {
		heap->add(0, f);
		hVal[f] = 0;
	}

	for (int iOp = 0; iOp < m->numPrecLessActions; iOp++) {
		int op = m->precLessActions[iOp];
		for (int iAdd = 0; iAdd < m->numAdds[op]; iAdd++) {
			int f = m->addLists[op][iAdd];
			hVal[f] = m->actionCosts[op];
			heap->add(hVal[f], f);
		}
	}
	while (!heap->isEmpty()) {
		int pVal = heap->topKey();
		int prop = heap->topVal();
		heap->pop();
		if (hVal[prop] < pVal)
			continue;
		for (int iOp = 0; iOp < m->precToActionSize[prop]; iOp++) {
			int op = m->precToAction[prop][iOp];
			if ((maxPrec[op] == UNREACHABLE)
					|| (hVal[maxPrec[op]] < hVal[prop])) {
				maxPrec[op] = prop;
			}
			if (--unsatPrecs[op] == 0) {
				for (int iF = 0; iF < m->numAdds[op]; iF++) {
					int f = m->addLists[op][iF];
					if ((hVal[f] == UNREACHABLE)
							|| ((hVal[maxPrec[op]] + costs[op]) < hVal[f])) {
						hVal[f] = hVal[maxPrec[op]] + costs[op];
						heap->add(hVal[f], f);
					}
				}
			}
		}
	}

	int res = INT_MIN;
	for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
		if (hVal[f] == UNREACHABLE) {
			res = UNREACHABLE;
			maxPrecG = -1;
			break;
		} else if (res < hVal[f]) {
			res = hVal[f];
			maxPrecG = f;
		}
	}

	return res;
}

int hsLmCut::updateHMax(noDelIntSet& g, bucketSet* cut) {
	heap->clear();
	for (int op = cut->getFirst(); op >= 0; op = cut->getNext()) {
		for (int iF = 0; iF < m->numAdds[op]; iF++) {
			int f = m->addLists[op][iF];
			if ((hVal[maxPrec[op]] + costs[op]) < hVal[f]) { // that f might be cheaper now
				hVal[f] = hVal[maxPrec[op]] + costs[op];
				heap->add(hVal[f], f);
			}
		}
	}

	while (!heap->isEmpty()) {
		int pVal = heap->topKey();
		int prop = heap->topVal();
		heap->pop();
		if (hVal[prop] < pVal) // we have prop's costs DECREASED -> this is fine
			continue;
		for (int iOp = 0; iOp < m->precToActionSize[prop]; iOp++) {
			int op = m->precToAction[prop][iOp];
			if ((unsatPrecs[op] == 0) && (prop == maxPrec[op])) { // this may change the costs of the operator and all its successors
				int opMaxPrec = -1;
				int val = INT_MIN;
				for (int iF = 0; iF < m->numPrecs[op]; iF++) {
					int f = m->precLists[op][iF];
					assert(hVal[f] != UNREACHABLE);
					if (hVal[f] > val) {
						opMaxPrec = f;
						val = hVal[f];
					}
				}
				maxPrec[op] = opMaxPrec;
				for (int iF = 0; iF < m->numAdds[op]; iF++) {
					int f = m->addLists[op][iF];
					if ((hVal[maxPrec[op]] + costs[op]) < hVal[f]) {
						hVal[f] = hVal[maxPrec[op]] + costs[op];
						assert(hVal[f] >= 0);
						heap->add(hVal[f], f);
					}
				}
			}
		}
	}

	int res = INT_MIN;
	for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
		if (hVal[f] == UNREACHABLE) {
			maxPrecG = -1;
			break;
		}
		if (res < hVal[f]) {
			res = hVal[f];
			maxPrecG = f;
		}
	}
	return res;
}
} /* namespace progression */
