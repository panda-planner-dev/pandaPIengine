//
// Created by dh on 28.02.20.
//

#include <cassert>
#include "LmCausal.h"

namespace progression {
    LmCausal::LmCausal(Model *htn) {
        if (!beTotallySilent)
            cout << "Init AND/OR landmark generator..." << endl;
        this->htn = htn;
        numNodes = htn->numStateBits + htn->numTasks + htn->numMethods;

        // init node set
        this->nodes = new LmAoNode *[numNodes];
        for (int i = 0; i < numNodes; i++) {
            this->nodes[i] = new LmAoNode(i);
            this->nodes[i]->containsFullSet = true;
            this->nodes[i]->maxSize = numNodes;
        }
        for (int i = 0; i < htn->numStateBits; i++) {
            nodes[fNode(i)]->nodeType = OR;
        }
        for (int i = 0; i < htn->numActions; i++) {
            nodes[tNode(i)]->nodeType = AND;
        }
        for (int i = 0; i < htn->numPrecLessActions; i++) {
            int a = htn->precLessActions[i];
            nodes[tNode(a)]->nodeType = INIT;
        }
        for (int i = htn->numActions; i < htn->numTasks; i++) {
            nodes[tNode(i)]->nodeType = OR;
        }
        for (int i = 0; i < htn->numMethods; i++) {
            nodes[mNode(i)]->nodeType = AND;
        }

        // build temporal graph
        auto *N = new vector<int>[numNodes];
        auto *Ninv = new vector<int>[numNodes];

        for (int iA = 0; iA < htn->numActions; iA++) {
            int nA = tNode(iA);
            for (int iF = 0; iF < htn->numPrecs[iA]; iF++) {
                int f = htn->precLists[iA][iF];
                int nF = fNode(f);
                N[nF].push_back(nA);
                Ninv[nA].push_back(nF);
            }
            for (int iF = 0; iF < htn->numAdds[iA]; iF++) {
                int f = htn->addLists[iA][iF];
                int nF = fNode(f);
                N[nA].push_back(nF);
                Ninv[nF].push_back(nA);
            }
        }

        for (int iM = 0; iM < htn->numMethods; iM++) {
            int nM = mNode(iM);
            int tAbs = htn->decomposedTask[iM];
            int nT = tNode(tAbs);
            N[nM].push_back(nT);
            Ninv[nT].push_back(nM);

            for (int iST = 0; iST < htn->numDistinctSTs[iM]; iST++) {
                int st = htn->sortedDistinctSubtasks[iM][iST];
                int nST = tNode(st);
                N[nST].push_back(nM);
                Ninv[nM].push_back(nST);
            }
        }

        // copy edges to nodes
        int maxSetSize = 0;
        for (int i = 0; i < numNodes; i++) {
            nodes[i]->numAffectedNodes = N[i].size();
            nodes[i]->affectedNodes = new int[N[i].size()];
            for (int j = 0; j < N[i].size(); j++) {
                nodes[i]->affectedNodes[j] = N[i][j];
            }
            nodes[i]->numAffectedBy = Ninv[i].size();
            if (nodes[i]->numAffectedBy > maxSetSize) {
                maxSetSize = nodes[i]->numAffectedBy;
            }
            nodes[i]->affectedBy = new int[Ninv[i].size()];
            for (int j = 0; j < Ninv[i].size(); j++) {
                nodes[i]->affectedBy[j] = Ninv[i][j];
            }
        }
        alreadyIn = new bool[numNodes];

        this->indices = new int[maxSetSize];
        this->numIndices = 0;

        delete[] N;
        delete[] Ninv;

        this->heap = new IntPairHeap<int>(numNodes / 4);
        this->setOperationHeap = new IntPairHeap<int>(25);
        this->unionSet = new noDelIntSet();
        this->unionSet->init(numNodes);


        if (!beTotallySilent)
            cout << "Calculate SCCs..." << endl;
        maxdfs = 0;
        U = new bool[numNodes];
        S = new vector<int>;
        containedS = new bool[numNodes];
        dfsI = new int[numNodes];
        lowlink = new int[numNodes];
        numSCCs = 0;
        nodeToSCC = new int[numNodes];
        for (int i = 0; i < numNodes; i++) {
            U[i] = true;
            containedS[i] = false;
            nodeToSCC[i] = -1;
        }

        tarjan(tNode(htn->initialTask));
        if (!beTotallySilent)
            cout << "Found " << numSCCs << " SCCs" << endl;
    }

    LmCausal::~LmCausal() {

    }

    void LmCausal::calcLMs(searchNode *n, planningGraph *pg) {
        this->pg = pg;
        this->pCalcLMs(n);
    }

    void LmCausal::calcLMs(searchNode *n) {
        this->pg = nullptr;
        this->pCalcLMs(n);
    }

    /*
     * LM calculation
     */
    void LmCausal::pCalcLMs(searchNode *tn) {
        int numUpdates = 0;
        int numNonUpdates = 0;

        for (int i = 0; i < numNodes; i++) {
            nodes[i]->containsFullSet = true;
            nodes[i]->numLMs = 0;
            alreadyIn[i] = false;
        }
        heap->clear();

        set<int> revert;
        for (int i = 0; i < htn->numStateBits; i++) {
            if (tn->state[i]) {
                int nIndex = this->fNode(i);
                heap->add(0, nIndex);
                alreadyIn[nIndex] = true;
                nodes[nIndex]->nodeType = INIT;
                revert.insert(nIndex);
            }
        }
        for (int i = 0; i < htn->numPrecLessActions; i++) {
            int a = htn->precLessActions[i];
            int nIndex = this->tNode(a);
            if (reachable(nIndex)) { // it might not be top down reachable
                heap->add(0, nIndex);
                alreadyIn[nIndex] = true;
            }
        }

        while (!heap->isEmpty()) {
            int ni = heap->topVal();
            //int metric = heap->topKey();
            heap->pop();
            alreadyIn[ni] = false;

            LmAoNode *n = nodes[ni];
            bool changed;
            if (n->nodeType == AND) {
                changed = setUnion(n, n->affectedBy, n->numAffectedBy);
            } else if (n->nodeType == OR) {
                changed = setIntersection(n, n->affectedBy, n->numAffectedBy);
            } else {
                assert(n->nodeType == INIT);
                changed = setToSelfLM(n);
            }
            if (changed) {
                //cout <<  metric << " node" << ni << " changed" << endl;
                numUpdates++;
                for (int i = 0; i < n->numAffectedNodes; i++) {
                    int affected = n->affectedNodes[i];
                    if ((reachable(affected)) && (!alreadyIn[affected])) {
                        //if (reachable(affected)) {
                        //int k = 0;
                        //int k = n->getSize();
                        //int k = nodeToSCC[affected];
                        int k = nodeToSCC[affected] * 10000 + n->getSize();
                        //int k = -1 * nodeToSCC[affected];
                        //int k = -1 * n->getSize();

                        heap->add(k, affected);
                        alreadyIn[affected] = true;
                    }
                }
            } else {
                // cout << metric <<  " node" << ni << " UNchanged" << endl;
                numNonUpdates++;
            }

        }

        // clean up
        for (int i : revert) {
            nodes[i]->nodeType = OR;
        }

        //cout << endl << "tasks in tn" << endl;
        copyGlobalLMsToResultSet(tn);

        if (!beTotallySilent) {
            for (int i = unionSet->getFirst(); i >= 0; i = unionSet->getNext()) {
                if (isTNode(i)) {
                    cout << "- lm T " << htn->taskNames[nodeToT(i)] << endl;
                } else if (isFNode(i)) {
                    cout << "- lm F " << htn->factStrs[nodeToF(i)] << endl;
                } else {
                    cout << "- lm M " << htn->methodNames[nodeToM(i)] << endl;
                }
            }
            cout << "Number of nodes " << numNodes << endl;
            cout << "Number of updates " << numUpdates << endl;
            cout << "Number of non-updates " << numNonUpdates << endl;
            cout << "Total " << (numNonUpdates + numUpdates) << endl;
            cout << "Number of landmarks " << this->numLMs << endl;
        }
    }

    void LmCausal::copyGlobalLMsToResultSet(const searchNode *tn) {
        unionSet->clear();
        for (int i = 0; i < tn->numContainedTasks; i++) {
            //cout << "- " << tn->containedTasks[i] << " node" << tNode(tn->containedTasks[i]) << " "
            //     << htn->taskNames[tn->containedTasks[i]] << endl;
            LmAoNode *n = nodes[tNode(tn->containedTasks[i])];
            if (n->containsFullSet) {
                for (int j = 0; j < numNodes; j++)
                    unionSet->insert(n->lms[j]);
            } else {
                for (int j = 0; j < n->numLMs; j++) {
                    unionSet->insert(n->lms[j]);
                }
            }
        }
        for (int i = 0; i < htn->gSize; i++) {
            int gf = htn->gList[i];
            LmAoNode *n = nodes[fNode(gf)];
            if (n->containsFullSet) {
                for (int j = 0; j < numNodes; j++)
                    unionSet->insert(n->lms[j]);
            } else {
                for (int j = 0; j < n->numLMs; j++) {
                    unionSet->insert(n->lms[j]);
                }
            }
        }

        numLMs = unionSet->getSize();
        landmarks = new landmark *[numLMs];
        int iLM = 0;
        for (int i = unionSet->getFirst(); i >= 0; i = unionSet->getNext()) {
            if (isTNode(i)) {
                landmarks[iLM] = new landmark(atom, task, 1);
                landmarks[iLM]->lm[0] = nodeToT(i);
            } else if (isFNode(i)) {
                landmarks[iLM] = new landmark(atom, fact, 1);
                landmarks[iLM]->lm[0] = nodeToF(i);
            } else {
                landmarks[iLM] = new landmark(atom, METHOD, 1);
                landmarks[iLM]->lm[0] = nodeToM(i);
            }
            iLM++;
        }
    }

    int LmCausal::fNode(int i) { return i; }

    int LmCausal::tNode(int i) { return htn->numStateBits + i; }

    int LmCausal::mNode(int i) { return htn->numStateBits + htn->numTasks + i; }

    int LmCausal::nodeToF(int i) { return i; }

    int LmCausal::nodeToT(int i) { return i - htn->numStateBits; }

    int LmCausal::nodeToM(int i) { return i - (htn->numStateBits + htn->numTasks); }

    bool LmCausal::isFNode(int i) { return (i < htn->numStateBits); }

    bool LmCausal::isTNode(int i) { return (i >= htn->numStateBits) && (i < (htn->numStateBits + htn->numTasks)); }

    bool LmCausal::isMNode(int i) {
        return ((i >= (htn->numStateBits + htn->numTasks)) && (i < htn->numStateBits + htn->numTasks + htn->numMethods));
    }

    bool LmCausal::setUnion(LmAoNode *update, int *combine, int size) {
        if (printDebugInfo)
            printSetOpInput(update, combine, size, "UNION");
        bool changed = false;

#ifndef NDEBUG
        // naive calculation for comparison
        set<int> DEBUGres;
        bool DEBUGisFullSet = false;
        int setCount = 0; // number of combined sets
        for (int i = 0; i < size; i++) {
            int iNode = combine[i];
            if (!this->reachable(iNode))
                continue;
            setCount++;
            if (nodes[iNode]->containsFullSet) {
                DEBUGisFullSet = true;
                break;
            } else {
                for (int i = 0; i < nodes[iNode]->getSize(); i++) {
                    DEBUGres.insert(nodes[iNode]->lms[i]);
                }
            }
        }
        DEBUGres.insert(update->ownIndex);
        bool DEBUGchanged = false;
        if (DEBUGisFullSet) {
            DEBUGchanged = !update->containsFullSet;
        } else {
            if (update->getSize() != DEBUGres.size()) {
                DEBUGchanged = true;
            } else {
                for (int lm : DEBUGres) {
                    if (!iu.containsInt(update->lms, 0, update->getSize() - 1, lm))
                        DEBUGchanged = true;
                }
            }
        }
#endif
        // is there a full set? -> result is full set
        bool containsFullSet = false;
        this->numIndices = 0;
        for (int i = 0; i < size; i++) {
            int iNode = combine[i];
            if (this->reachable(iNode)) {
                if (nodes[iNode]->containsFullSet) {
                    containsFullSet = true;
                    break;
                } else {
                    indices[numIndices++] = iNode;
                }
            }
        }
        if (containsFullSet) {
            if (!update->containsFullSet) {
                changed = true;
                update->containsFullSet = true;
            }
        } else if (numIndices == 0) {
            changed = !((!update->containsFullSet) && (update->numLMs == 1) && (update->lms[0] == update->ownIndex));
            resizeIfSmallerNOCOPY(update, 1);
            update->lms[0] = update->ownIndex;
            update->numLMs = 1;
            update->containsFullSet = false;
            //} else if (numIndices == 1) {
            //    changed = copyLms(nodes[indices[0]], update);
        } else {
            update->containsFullSet = false;
            unionSet->clear();
            unionSet->insert(update->ownIndex);
            for (int i = 0; i < numIndices; i++) {
                LmAoNode *n = nodes[indices[i]];
                for (int j = 0; j < n->numLMs; j++) {
                    unionSet->insert(n->lms[j]);
                }
            }
            unionSet->sort(); // result must be sorted
            resizeIfSmallerNOCOPY(update, unionSet->getSize());
            int i = 0;
            for (int lm = unionSet->getFirst(); lm >= 0; lm = unionSet->getNext()) {
                if (update->lms[i] != lm) {
                    changed = true;
                    update->lms[i] = lm;
                }
                i++;
            }
            if (update->numLMs != unionSet->getSize()) {
                update->numLMs = unionSet->getSize();
                changed = true;
            }
        }
        if (printDebugInfo) {
            cout << "Result ";
            update->print();
            cout << endl;
            if (changed)
                cout << "changed = TRUE" << endl;
            else
                cout << "changed = FALSE" << endl;
        }

#ifndef NDEBUG
        assert(changed == DEBUGchanged);
        assert(DEBUGisFullSet == update->containsFullSet);
        if (!update->containsFullSet) {
            assert(update->getSize() == DEBUGres.size());
            for (int lm : DEBUGres) assert(iu.containsInt(update->lms, 0, update->getSize() - 1, lm));
        }
#endif
        return changed;
    }

    void
    LmCausal::printSetOpInput(LmAoNode *nodeToUpdate, const int *nodesToCombine, int size, string setOperation) const {
        cout << endl << "updating ";
        nodeToUpdate->print();
        cout << endl << "to " << setOperation << " of" << endl;
        for (int i = 0; i < size; i++) {
            cout << "- ";
            nodes[nodesToCombine[i]]->print();
            cout << endl;
        }
    }

    bool LmCausal::setIntersection(LmAoNode *update, int *combine, int size) {
        if (printDebugInfo)
            printSetOpInput(update, combine, size, "INTERSECTION");
        bool changed = false;

#ifndef NDEBUG
        // naive calculation for comparison
        set<int> DEBUGres;
        bool DEBUGisFullSet = true;
        int setCount = 0; // number of combined sets
        for (int i = 0; i < size; i++) {
            int iNode = combine[i];
            if (!this->reachable(iNode))
                continue;
            setCount++;
            if (nodes[iNode]->containsFullSet)
                continue; // result remains as it is
            else {
                if (DEBUGisFullSet) {
                    DEBUGisFullSet = false;
                    for (int i = 0; i < nodes[iNode]->getSize(); i++) {
                        DEBUGres.insert(nodes[iNode]->lms[i]);
                    }
                } else {
                    set<int> second;
                    for (int i = 0; i < nodes[iNode]->getSize(); i++) {
                        int lm = nodes[iNode]->lms[i];
                        if (DEBUGres.find(lm) != DEBUGres.end())
                            second.insert(lm);
                    }
                    DEBUGres.clear();
                    for(int lm : second) DEBUGres.insert(lm);
                }
            }
        }
        if(setCount == 0)
            DEBUGisFullSet = false;

        DEBUGres.insert(update->ownIndex);
        bool DEBUGchanged = false;
        if (DEBUGisFullSet) {
            DEBUGchanged = !update->containsFullSet;
        } else {
            if (update->getSize() != DEBUGres.size()) {
                DEBUGchanged = true;
            } else {
                for (int lm : DEBUGres) {
                    if (!iu.containsInt(update->lms, 0, update->getSize() - 1, lm))
                        DEBUGchanged = true;
                }
            }
        }
#endif

        // filter unreachable and sort ascending by set size
        this->setOperationHeap->clear();
        for (int i = 0; i < size; i++) {
            int iNode = combine[i];
            if (this->reachable(iNode)) {
                setOperationHeap->add(nodes[iNode]->getSize(), iNode);
            }
        }
        this->numIndices = setOperationHeap->size();
        for (int i = 0; i < this->numIndices; i++) {
            indices[i] = setOperationHeap->topVal();
            setOperationHeap->pop();
        }

        // create intersection
        if (numIndices == 0) {
            // todo: is this correct?
            if (!update->containsFullSet) {
                changed = true;
                update->containsFullSet = true;
            }
        } else if (nodes[indices[0]]->containsFullSet) {
            if (!update->containsFullSet) {
                changed = true;
                update->containsFullSet = true;
            }
        } else {
            update->containsFullSet = false;
            bool addedOwnIndex = false;
            int resultIndex = 0;
            LmAoNode *n = nodes[indices[0]];
            for (int i = 0; i < n->numLMs; i++) {
                int lm = n->lms[i];
                bool contained = true;
                if (lm == update->ownIndex) { // will be added anyway
                    addedOwnIndex = true;
                } else { // need to check if it is contained in all sets
                    for (int j = 1; j < numIndices; j++) {
                        if (!nodes[indices[j]]->contains(lm)) {
                            contained = false;
                            break;
                        }
                    }
                }
                // add to updated set
                if ((!addedOwnIndex) && (lm > update->ownIndex)) {
                    addedOwnIndex = true;
                    ensureSizeCopy(update, resultIndex + 1);
                    if (update->lms[resultIndex] != update->ownIndex) {
                        changed = true;
                        update->lms[resultIndex] = update->ownIndex;
                    }
                    resultIndex++;
                }
                if (contained) {
                    ensureSizeCopy(update, resultIndex + 1);
                    if (update->lms[resultIndex] != lm) {
                        changed = true;
                        update->lms[resultIndex] = lm;
                    }
                    resultIndex++;
                }
            }
            if (!addedOwnIndex) {
                ensureSizeCopy(update, resultIndex + 1);
                if (update->lms[resultIndex] != update->ownIndex) {
                    changed = true;
                    update->lms[resultIndex] = update->ownIndex;
                }
                resultIndex++;
            }
            if (update->numLMs != resultIndex) {
                update->numLMs = resultIndex;
                changed = true;
            }
        }
        if (printDebugInfo) {
            cout << "Result ";
            update->print();
            cout << endl;
            if (changed)
                cout << "changed = TRUE" << endl;
            else
                cout << "changed = FALSE" << endl;
        }
#ifndef NDEBUG
        assert(changed == DEBUGchanged);
        assert(DEBUGisFullSet == update->containsFullSet);
        if (!update->containsFullSet) {
            assert(update->getSize() == DEBUGres.size());
            for (int lm : DEBUGres) assert(iu.containsInt(update->lms, 0, update->getSize() - 1, lm));
        }
#endif
        return changed;
    }

    bool LmCausal::setToSelfLM(LmAoNode *update) {
        bool changed = !((!update->containsFullSet) && (update->numLMs == 1) && (update->lms[0] == update->ownIndex));
        if (changed) {
            ensureSizeCopy(update, 1);
            update->numLMs = 1;
            update->lms[0] = update->ownIndex;
            update->containsFullSet = false;
        }

        if (printDebugInfo) {
            cout << endl << "Initial node was set to ";
            update->print();
            cout << endl;
            if (changed)
                cout << "changed = TRUE" << endl;
            else
                cout << "changed = FALSE" << endl;
        }

        return changed;
    }

    void LmCausal::ensureSizeCopy(LmAoNode *node, int size) {
        if (node->lmContainerSize < size) {
            int *temp = new int[size * 2];
            if (node->lms != nullptr) {
                for (int i = 0; i < node->lmContainerSize; i++) {
                    temp[i] = node->lms[i];
                }
                for (int i = node->lmContainerSize; i < (size * 2); i++) {
                    temp[i] = -1;
                }
                delete[] node->lms;
            } else {
                for (int i = 0; i < (size * 2); i++) {
                    temp[i] = -1;
                }
            }
            node->lms = temp;
            node->lmContainerSize = size * 2;
        }
    }

    void LmCausal::resizeIfSmallerNOCOPY(LmAoNode *node, int size) {
        if (node->lmContainerSize < size) {
            if (node->lms != nullptr) {
                delete[] node->lms;
            }
            node->lms = new int[size];
            for (int i = 0; i < size; i++)
                node->lms[i] = -1;
            node->lmContainerSize = size;
        }
    }

    bool LmCausal::reachable(int nodeIndex) {
        if (pg == nullptr)
            return true;
        else if (isTNode(nodeIndex)) {
            return pg->taskReachable(nodeToT(nodeIndex));
        } else if (isFNode(nodeIndex)) {
            return pg->usefulFactSet.get(nodeToF(nodeIndex));
        } else {
            assert (isMNode(nodeIndex));
            return pg->methodReachable(nodeToM(nodeIndex));
        }
    }

    void LmCausal::tarjan(int v) {
        dfsI[v] = maxdfs;
        lowlink[v] = maxdfs; // v.lowlink <= v.dfs
        maxdfs++;

        S->push_back(v);
        containedS[v] = true;
        U[v] = false; // delete v from U

        for (int iST = 0; iST < nodes[v]->numAffectedBy; iST++) {
            int v2 = nodes[v]->affectedBy[iST];
            if (U[v2]) {
                tarjan(v2);
                if (lowlink[v] > lowlink[v2]) {
                    lowlink[v] = lowlink[v2];
                }
            } else if (containedS[v2]) {
                if (lowlink[v] > dfsI[v2])
                    lowlink[v] = dfsI[v2];
            }
        }

        if (lowlink[v] == dfsI[v]) { // root of an SCC
            int v2;
            do {
                v2 = S->back();
                S->pop_back();
                containedS[v2] = false;
                nodeToSCC[v2] = numSCCs;
            } while (v2 != v);
            numSCCs++;
        }
    }

    void LmCausal::prettyprintAndOrGraph() {
        cout << "digraph {" << endl;
        for (int i = 0; i < numNodes; i++) {
            if (i == 0) {
                cout << "    subgraph cluster_STATE {" << endl;
            } else if (i == htn->numStateBits) {
                cout << "    }" << endl;
                cout << "    subgraph cluster_ACTIONS {" << endl;
            } else if (i == (htn->numStateBits + htn->numActions)) {
                cout << "    }" << endl;
            }
            cout << "    n" << i << "[";

            if (nodes[i]->nodeType == AND) {
                cout << "shape=box";
            } else if (nodes[i]->nodeType == OR) {
                cout << "shape=circle";
            } else { // this is an init node
                cout << "shape=diamond";
            }
            cout << ", ";

            if (isFNode(i)) {
                cout << "label=\"" << htn->factStrs[nodeToF(i)] << "\"";
            } else if (isTNode(i)) {
                cout << "label=\"" << htn->taskNames[nodeToT(i)] << "\"";
            } else if (isMNode(i)) {
                cout << "label=\"" << htn->methodNames[nodeToM(i)] << "\"";
            } else {
                cout << "label=\"\""; // empty
            }
            //cout << "label=\"" << i << "\"";

            cout << "]" << endl;
        }
        cout << endl;
        for (int i = 0; i < numNodes; i++) {
            for (int j = 0; j < (int) nodes[i]->numAffectedNodes; j++) {
                cout << "    n" << i << " -> " << "n" << nodes[i]->affectedNodes[j] << ";" << endl;
            }
        }

        cout << "}" << endl;
    }

    int LmCausal::getNumLMs() {
        return this->numLMs;
    }

    landmark **LmCausal::getLMs() {
        return landmarks;
    }

    /*
    bool LmCausal::copyLms(LmAoNode *from, LmAoNode *to) {
        bool changed = false;
        this->ensureSizeCopy(to, from->numLMs);
        int toIndex = 0;
        for(int i =0; i < from->numLMs; i++) {
            if(to->lms[i] != from->lms[toIndex]) {
                changed = true;
                to->lms[i] == from->lms[toIndex];
            }
            toIndex++;
        }

        update->lms[0] = update->ownIndex;
        update->numLMs = 1;
        update->containsFullSet = false;

        return changed;
    }*/

    void LmCausal::prettyPrintLMs() {
        for (int i = 0; i < this->numLMs; i++) {
            landmark *lm = landmarks[i];
            cout << "- LM ";
            string *nameStrs = nullptr;
            if (lm->type == fact) {
                cout << "fact";
                nameStrs = htn->factStrs;
            } else if (lm->type == task) {
                cout << "task";
                nameStrs = htn->taskNames;
            } else if (lm->type == METHOD) {
                cout << "meth";
                nameStrs = htn->methodNames;
            }
            cout << " ";

            if (lm->connection == atom) {
                cout << "atom";
            } else if (lm->connection == conjunctive) {
                cout << "conj";
            } else if (lm->connection == disjunctive) {
                cout << "disj";
            }
            for (int j = 0; j < lm->size; j++) {
                cout << " " << nameStrs[lm->lm[j]];
            }
            cout << endl;
        }
    }

    void LmCausal::initIterTask(int task) {
        this->iterN = this->nodes[tNode(task)];
        this->iterI = 0;
    }

    void LmCausal::initIterFact(int fact) {
        this->iterN = this->nodes[fNode(fact)];
        this->iterI = 0;
    }

    void LmCausal::initIterMethod(int method) {
        this->iterN = this->nodes[mNode(method)];
        this->iterI = 0;
    }

    int LmCausal::iterGetLm() {
        int lm = this->iterN->lms[this->iterI];
        if (isTNode(lm)) {
            return nodeToT(lm);
        } else if (isFNode(lm)) {
            return nodeToF(lm);
        } else {
            return nodeToM(lm);
        }
    }

    lmType LmCausal::iterGetLmType() {
        int lm = this->iterN->lms[this->iterI];
        if (isTNode(lm)) {
            return task;
        } else if (isFNode(lm)) {
            return fact;
        } else {
            assert(isMNode(lm));
            return METHOD;
        }
    }

    bool LmCausal::iterHasNext() {
        if (this->iterN->containsFullSet)
            return false;
        return (this->iterI < this->iterN->getSize());
    }

    void LmCausal::iterate() {
        this->iterI++;
    }
}
