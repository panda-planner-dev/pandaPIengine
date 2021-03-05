/*
 * sAddFF.cpp
 *
 *  Created on: 23.09.2017
 *      Author: Daniel Höller
 */

#include "hsAddFF.h"
#include <cassert>
#include <cstring>

#ifdef false
namespace progression {

    hsAddFF::hsAddFF(Model *sas) {
        m = sas;
        set<int> s;
        for (int i = 0; i < m->numActions; i++) {
            s.clear();
            for (int j = 0; j < m->numPrecs[i]; j++) {
                s.insert(m->precLists[i][j]);
                assert(m->precLists[i][j] < m->numStateBits);
            }
            assert(s.size() == m->numPrecs[i]);

            s.clear();
            for (int j = 0; j < m->numAdds[i]; j++) {
                s.insert(m->addLists[i][j]);
                assert(m->addLists[i][j] < m->numStateBits);
            }
            assert(s.size() == m->numAdds[i]);
        }

        s.clear();
        for (int j = 0; j < m->s0Size; j++) {
            s.insert(m->s0List[j]);
            assert(m->s0List[j] < m->numStateBits);
        }
        assert(s.size() == m->s0Size);

        s.clear();
        for (int j = 0; j < m->gSize; j++) {
            s.insert(m->gList[j]);
            assert(m->gList[j] < m->numStateBits);
        }
        assert(s.size() == m->gSize);


        heuristic = sasFF;
        assert(!sas->isHtnModel);
        hValPropInit = new tHVal[m->numStateBits];
        for (int i = 0; i < m->numStateBits; i++) {
            hValPropInit[i] = tHValUNR;
        }
        queue = new IntPairHeap(m->numStateBits * 2);
        numSatPrecs = new int[m->numActions];
        hValOp = new tHVal[m->numActions];
        hValProp = new tHVal[m->numStateBits];
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
            if (hValProp[f] == tHValUNR) {
                cout << "f=" << f << " " << m->factStrs[f] << endl;
            }
            assert(hValProp[f] != tHValUNR);
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
        tHVal hVal = tHValUNR;

        memcpy(numSatPrecs, m->numPrecs, sizeof(int) * m->numActions);
        memcpy(hValProp, hValPropInit, sizeof(tHVal) * m->numStateBits);
        for (int i = 0; i < m->numActions; i++) {
            hValOp[i] = m->actionCosts[i];
        }

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
            tHVal pVal = queue->topKey();
            tHVal prop = queue->topVal();
            assert(pVal >= 0);
            queue->pop();
            if (hValProp[prop] < pVal)
                continue;
            if (prop == 1391)
                cout << "ahah " << hValProp[prop] << endl;

            if (g.get(prop)) {
                cout << "g:" << prop << endl;
                if (--numGoals == 0) {
                    bool some = false;
                    for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
                        if (hValProp[f] == tHValUNR) {
                            some = true;
                        }
                    }
                    if (some) {
                        cout << "here!" << endl;
                        cout << g.getSize() << endl;
                        set<int> s;
                        for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
                            assert(f < m->numStateBits);
                            cout << " " << f;
                            if (hValProp[f] == tHValUNR) {
                                cout << "!";
                            }
                            s.insert(f);
                        }
                        assert(s.size() == g.getSize());
                        cout << endl;
                    }

                    if (heuristic == sasAdd) {
                        hVal = 0;
                        for (int f = g.getFirst(); f >= 0; f = g.getNext()) {
                            assert(hValProp[f] != tHValUNR);
                            hVal += hValProp[f];
                        }
                        break;
                    } else { // FF extraction
                        hVal = getFF(g, hVal);
                        break;
                    }
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
                            if (f == 1391) {
                                cout << "ahah1 " << hValProp[f] << endl;
                                cout << op << " " << m->taskNames[op] << endl;
                                for(int i = 0; i < m->numPrecs[op]; i++) {
                                    cout<<"- " << m->factStrs[m->precLists[op][i]] << endl;
                                }
                            }
                        }
                    }
                }
            }
        }
        cout << "----------------" << endl;
        if (hVal == tHValUNR) {
            return UNREACHABLE;
        } else if (hVal >= INT_MAX) {
            if (this->firstOverflow) {
                cout << "- WARNING: There has been an overflow of the heuristic value." << endl;
            }
            return 10000000;
        } else {
            return (int) hVal;
        }
    }

}
/* namespace progression */
#endif