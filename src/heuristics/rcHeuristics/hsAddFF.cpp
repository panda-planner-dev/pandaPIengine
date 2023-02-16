/*
 * sAddFF.cpp
 *
 *  Created on: 23.09.2017
 *      Author: Daniel Höller
 */

#include "hsAddFF.h"
#include <cassert>
#include <cstring>

namespace progression {

    hsAddFF::hsAddFF(Model *sas) {
        heuristic = sasFF;
        assert(!sas->isHtnModel);
        m = sas;
        hValPropInit = new hType[m->numStateBits];
        for (int i = 0; i < m->numStateBits; i++) {
            hValPropInit[i] = hUnreachable;
        }
        queue = new IntPairHeap<hType>(m->numStateBits * 2);
        numSatPrecs = new int[m->numActions];
        hValOp = new hType[m->numActions];
        hValProp = new hType[m->numStateBits];
        reachedBy = new int[m->numStateBits];
        markedFs.init(m->numStateBits);
        markedOps.init(m->numActions);
        needToMark.init(m->numStateBits);
        allActionsCostOne = true;
        for (int i = 0; i < m->numActions; i++) {
            if (m->actionCosts[i] != 1) {
                allActionsCostOne = false;
                break;
            }
        }
        assert(assertPrecAddDelSets());
    }

    bool hsAddFF::assertPrecAddDelSets() {
        for (int i = 0; i < m->numActions; i++) {
            set<int> testset;
            for (int j = 0; j < m->numPrecs[i]; j++) {
                testset.insert(m->precLists[i][j]);
            }
            assert(m->numPrecs[i] == testset.size());
            testset.clear();

            for (int j = 0; j < m->numAdds[i]; j++) {
                testset.insert(m->addLists[i][j]);
            }
            assert(m->numAdds[i] == testset.size());
            testset.clear();

            for (int j = 0; j < m->numDels[i]; j++) {
                testset.insert(m->delLists[i][j]);
            }
            assert(m->numDels[i] == testset.size());
        }
        return true;
    }

    hsAddFF::~hsAddFF() {
        delete[] hValPropInit;
        delete[] numSatPrecs;
        delete[] hValOp;
        delete[] hValProp;
        delete[] reachedBy;
        delete queue;
    }

    hType hsAddFF::getFF(noDelIntSet &g) {
        // FF extraction
        markedFs.clear();
        markedOps.clear();
        needToMark.clear();
        for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
            assert(hValProp[f] != hUnreachable);
            needToMark.push(f);
            while (!needToMark.isEmpty()) {
                int someF = needToMark.pop();
                if (markedFs.get(someF)) {
                    continue;
                }
                markedFs.insert(someF);
                if (reachedBy[someF] != NOACTION) {
                    // else it is set in s0
                    for (int i = 0; i < m->numPrecs[reachedBy[someF]]; i++) {
                        int prec = m->precLists[reachedBy[someF]][i];
                        needToMark.push(prec);
                    }
                    markedOps.insert(reachedBy[someF]);
                }
            }
        }
        if (allActionsCostOne) {
            return markedOps.getSize();
        } else {
            hType hVal = 0;
            for (int op = markedOps.getFirst(); op >= 0; op = markedOps.getNext()) {
                hVal += m->actionCosts[op];
            }
            return hVal;
        }
    }

    int hsAddFF::getHeuristicValue(bucketSet &s, noDelIntSet &g) {
        if (g.getSize() == 0)
            return 0;
        hType hVal = hUnreachable;

        memcpy(numSatPrecs, m->numPrecs, sizeof(int) * m->numActions);
        for (int i = 0; i < m->numActions; i++) {
            hValOp[i] = m->actionCosts[i];
        }
        memcpy(hValProp, hValPropInit, sizeof(hType) * m->numStateBits);

        int numGoals = g.getSize();

        queue->clear();
        for (int f = s.removeFirst(); f >= 0; f = s.removeNext()) {
            queue->add(0, f);
            hValProp[f] = 0;
            reachedBy[f] = NOACTION;
        }

        for (int i = 0; i < m->numPrecLessActions; i++) {
            int ac = m->precLessActions[i];
            for (int iAdd = 0; iAdd < m->numAdds[ac]; iAdd++) {
                int fAdd = m->addLists[ac][iAdd];
                hValProp[fAdd] = m->actionCosts[ac];
                reachedBy[fAdd] = ac;
                queue->add(hValProp[fAdd], fAdd);
            }
        }
        while (!queue->isEmpty()) {
            hType pVal = queue->topKey();
            assert(pVal >= 0);
            int prop = queue->topVal();
            queue->pop();
            if (hValProp[prop] < pVal)
                continue;
            if (g.get(prop)) {
                if (--numGoals == 0) {
                    if (heuristic == sasAdd) {
                        hVal = 0;
                        for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
                            assert(hValProp[f] != hUnreachable);
                            hVal += hValProp[f];
                        }
                        break;
                    } else { // FF extraction
                        hVal = getFF(g);
                        break;
                    }
                }
            }
            for (int iOp = 0; iOp < m->precToActionSize[prop]; iOp++) {
                int op = m->precToAction[prop][iOp];
                hType newVal;
                if (this->heuristic == sasFF) {
                    newVal = max(hValOp[op], m->actionCosts[op] + pVal);
                }
                else {
                    newVal = hValOp[op] + pVal;
                }

                if ((newVal < hValOp[op]) || (newVal < pVal)) {
                    if (!this->reportedOverflow) {
                        cout << "WARNING: Integer overflow in hAdd/hFF calculation. Value has been cut." << endl;
                        cout << "         You can choose a different data type for the Add/FF calculation (look for \"hType\" in heuristic class)" << endl;
                        cout << "         This message will only be reported once!" << endl;
                        this->reportedOverflow = true;
                    }
                    return INT_MAX - 2; // here, the external data type for heuristic values (i.e. int) must be used
                }
                hValOp[op] = newVal;

                assert(hValOp[op] >= 0);
                if (--numSatPrecs[op] == 0) {
                    for (int iF = 0; iF < m->numAdds[op]; iF++) {
                        int f = m->addLists[op][iF];
                        if (hValOp[op] < hValProp[f]) {
                            hValProp[f] = hValOp[op];
                            reachedBy[f] = op; // only used by FF
                            queue->add(hValProp[f], f);
                        }
                    }
                }
            }
        }
        if (hVal == hUnreachable) {
            return UNREACHABLE;
        } else if (hVal >= INT_MAX) {
            if (!this->reportedOverflow) {
                cout << "WARNING: Integer overflow in hAdd/hFF calculation. Value has been cut." << endl;
                cout << "         You can choose a different data type for the Add/FF calculation (look for \"hType\" in heuristic class)" << endl;
                cout << "         This message will only be reported once!" << endl;
                this->reportedOverflow = true;
            }
            return INT_MAX - 2;
        } else {
            return (int) hVal;
        }
    }

}
/* namespace progression */
