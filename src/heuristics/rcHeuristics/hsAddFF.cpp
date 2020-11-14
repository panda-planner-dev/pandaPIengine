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
    }

    hsAddFF::~hsAddFF() {
        delete[] hValPropInit;
        delete[] numSatPrecs;
        delete[] hValOp;
        delete[] hValProp;
        delete[] reachedBy;
        delete queue;
    }

    int hsAddFF::getFF(noDelIntSet &g, int hVal) {
        // FF extraction
        markedFs.clear();
        markedOps.clear();
        needToMark.clear();
        for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
            assert(hValProp[f] != UNREACHABLE);
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
            hVal = 0;
            for (int op = markedOps.getFirst(); op >= 0; op = markedOps.getNext()) {
                //cout << m->taskNames[op] << endl;
                hVal += m->actionCosts[op];
            }
            return hVal;
        }
    }

    int hsAddFF::getHeuristicValue(bucketSet &s, noDelIntSet &g) {
        if (g.getSize() == 0)
            return 0;
        int hVal = UNREACHABLE;

        memcpy(numSatPrecs, m->numPrecs, sizeof(int) * m->numActions);
        memcpy(hValOp, m->actionCosts, sizeof(int) * m->numActions);
        memcpy(hValProp, hValPropInit, sizeof(int) * m->numStateBits);

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
                queue->add(hValProp[fAdd], fAdd);
            }
        }
        while (!queue->isEmpty()) {
            int pVal = queue->topKey();
            int prop = queue->topVal();
            queue->pop();
            if (hValProp[prop] < pVal)
                continue;
            if (g.get(prop))
                if (--numGoals == 0) {
                    if (heuristic == sasAdd) {
                        hVal = 0;
                        for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
                            assert(hValProp[f] != UNREACHABLE);
                            hVal += hValProp[f];
                        }
                        break;
                    } else { // FF extraction
                        hVal = getFF(g, hVal);
                        break;
                    }
                }
            for (int iOp = 0; iOp < m->precToActionSize[prop]; iOp++) {
                int op = m->precToAction[prop][iOp];
                hValOp[op] += pVal;
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

        return hVal;
    }

}
/* namespace progression */
