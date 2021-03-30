/*
 * hhDOfree.cpp
 *
 *  Created on: 27.09.2018
 *      Author: Daniel Höller
 */

#include "hhDOfree.h"

hhDOfree::hhDOfree(Model *htn, searchNode *n, int index, IloNumVar::Type IntType, IloNumVar::Type BoolType, csSetting IlpSetting,
                   csTdg tdgConstrs, csPg pgConstrs, csAndOrLms aoLMConstrs, csLmcLms lmcLMConstrs,
                   csNetChange ncConstrs, csAddExternalLms addLMConstrs) :
        Heuristic(htn, index),
		cIntType(IntType),
        cBoolType(BoolType),
        cSetting(IlpSetting),
        cTdg(tdgConstrs),
        cPg(pgConstrs),
        cAndOrLms(aoLMConstrs),
        cLmcLms(lmcLMConstrs),
        cNetChange(ncConstrs),
        cAddExternalLms(addLMConstrs) {

    assert((cIntType == IloNumVar::Float) || (cIntType == IloNumVar::Int));
    assert((cBoolType == IloNumVar::Float) || (cBoolType == IloNumVar::Bool));

    if ((cIntType == IloNumVar::Int) && (cBoolType == IloNumVar::Bool)) {
        cout << "- ILP calculation: Integer and Boolean variables in the ILP are not relaxed" << endl;
    } else if ((cIntType == IloNumVar::Float) && (cBoolType == IloNumVar::Float)) {
        cout << "- LP calculation: Integer and Boolean variables in the ILP are relaxed to Float variables" << endl;
    } else {
        if (cIntType == IloNumVar::Float) {
            cout << "- Integer variables in the ILP are relaxed to Float variables" << endl;
        } else if (cIntType == IloNumVar::Int) {
            cout << "- Integer variables in the ILP are not relaxed" << endl;
        } else {
            cout << "UNKNOWN TYPE FOR ILP INT VARS" << endl;
            exit(-1);
        }
        if (cBoolType == IloNumVar::Float) {
            cout << "- Boolean variables in the ILP are relaxed to Float variables" << endl;
        } else if (cBoolType == IloNumVar::Bool) {
            cout << "- Boolean variables in the ILP are not relaxed" << endl;
        } else {
            cout << "UNKNOWN TYPE FOR ILP BOOL VARS" << endl;
            exit(-1);
        }
    }

    preProReachable.init(htn->numTasks);
    this->pg = new planningGraph(htn);
    this->htn = htn;

    // init
    cout << "- initializing data structures for ILP model ... ";
    this->iUF = new int[htn->numStateBits];
    this->iUA = new int[htn->numTasks];
    this->iM = new int[htn->numMethods];

    int curI = 0;
    EStartI = new int[htn->numActions];

    // storing for a certain state feature the action that has it as add effect
    vector<int> * EInvAction = new vector<int>[htn->numStateBits];
    vector<int> * EInvEffIndex = new vector<int>[htn->numStateBits];

    for (int a = 0; a < htn->numActions; a++) {
        EStartI[a] = curI; // set first index of this action
        curI += htn->numAdds[a]; // increase by number of add effects
        for (int ai = 0; ai < htn->numAdds[a]; ai++) {
            EInvAction[htn->addLists[a][ai]].push_back(a); // store action index
            EInvEffIndex[htn->addLists[a][ai]].push_back(ai); // store effect index
        }
    }

    // store to permanent representation
    this->iE = new int[EStartI[htn->numActions - 1]
                       + htn->numAdds[htn->numActions - 1]]; // the add effects
    EInvSize = new int[htn->numStateBits];
    iEInvActionIndex = new int *[htn->numStateBits];
    iEInvEffIndex = new int *[htn->numStateBits];

    for (int i = 0; i < htn->numStateBits; i++) {
        EInvSize[i] = EInvAction[i].size();
        iEInvActionIndex[i] = new int[EInvSize[i]];
        iEInvEffIndex[i] = new int[EInvSize[i]];
        for (int j = 0; j < EInvSize[i]; j++) {
            iEInvActionIndex[i][j] = EInvAction[i].back();
            EInvAction[i].pop_back();
            iEInvEffIndex[i][j] = EInvEffIndex[i].back();
            EInvEffIndex[i].pop_back();
        }
    }

	delete[] EInvAction;
	delete[] EInvEffIndex;

    this->iTP = new int[htn->numStateBits]; // time proposition
    this->iTA = new int[htn->numActions]; // time action
    cout << "done." << endl;

    if (this->cTdg == cTdgFull) {
        cout << "- initializing data structures to prevent disconnected components... ";
        sccNumIncommingMethods = new int *[htn->numSCCs];
        sccIncommingMethods = new int **[htn->numSCCs];

        // compute incomming methods for each scc
        for (int i = 0; i < htn->numCyclicSccs; i++) {
            int sccTo = htn->sccsCyclic[i];

            sccNumIncommingMethods[sccTo] = new int[htn->sccSize[sccTo]];
            for (int j = 0; j < htn->sccSize[sccTo]; j++) {
                sccNumIncommingMethods[sccTo][j] = 0;
            }
            sccIncommingMethods[sccTo] = new int *[htn->sccSize[sccTo]];

            set<int> * incomingMethods = new set<int>[htn->sccSize[sccTo]];
            for (int j = 0; j < htn->sccGnumPred[sccTo]; j++) {
                int sccFrom = htn->sccGinverse[sccTo][j];
                for (int iTFrom = 0; iTFrom < htn->sccSize[sccFrom]; iTFrom++) {
                    int tFrom = htn->sccToTasks[sccFrom][iTFrom];
                    for (int iM = 0; iM < htn->numMethodsForTask[tFrom]; iM++) {
                        int method = htn->taskToMethods[tFrom][iM];
                        for (int iST = 0; iST < htn->numSubTasks[method]; iST++) {
                            int subtask = htn->subTasks[method][iST];
                            for (int iTTo = 0; iTTo < htn->sccSize[sccTo]; iTTo++) {
                                int tTo = htn->sccToTasks[sccTo][iTTo];
                                if (subtask == tTo) {
                                    incomingMethods[iTTo].insert(method);
                                }
                            }
                        }
                    }
                }
            }

            // copy to permanent data structures
            int sumSizes = 0;
            for (int k = 0; k < htn->sccSize[sccTo]; k++) {
                sumSizes += incomingMethods[k].size();
                //int task = htn->sccToTasks[sccTo][k];
                // cout << htn->taskNames[task] << " reached by ";
                sccIncommingMethods[sccTo][k] = new int[incomingMethods[k].size()];
                sccNumIncommingMethods[sccTo][k] = incomingMethods[k].size();
                int l = 0;
                for (std::set<int>::iterator it = incomingMethods[k].begin();
                     it != incomingMethods[k].end(); it++) {
                    sccIncommingMethods[sccTo][k][l] = *it;
                    //cout << htn->methodNames[sccIncommingMethods[][task][l]] << " ";
                    l++;
                }
                //cout << endl;
            }

			delete[] incomingMethods;
            assert(sumSizes > 0); // some task out of the scc should be reached
        }

        sccNumInnerFromTo = new int *[htn->numSCCs];
        sccNumInnerToFrom = new int *[htn->numSCCs];
        sccNumInnerFromToMethods = new int **[htn->numSCCs];

        sccInnerFromTo = new int **[htn->numSCCs];
        sccInnerToFrom = new int **[htn->numSCCs];
        sccInnerFromToMethods = new int ***[htn->numSCCs];

        for (int i = 0; i < htn->numCyclicSccs; i++) {
            int scc = htn->sccsCyclic[i];

            sccNumInnerFromTo[scc] = new int[htn->sccSize[scc]];
            sccNumInnerToFrom[scc] = new int[htn->sccSize[scc]];
            sccNumInnerFromToMethods[scc] = new int *[htn->sccSize[scc]];

            sccInnerFromTo[scc] = new int *[htn->sccSize[scc]];
            sccInnerToFrom[scc] = new int *[htn->sccSize[scc]];
            sccInnerFromToMethods[scc] = new int **[htn->sccSize[scc]];

            set<int> ** reachability = new set<int>*[htn->sccSize[scc]];
            for (int iTo = 0; iTo < htn->sccSize[scc]; iTo++) reachability[iTo] = new set<int>[htn->sccSize[scc]];
            set<int> * tasksToFrom = new set<int>[htn->sccSize[scc]];
            set<int> * tasksFromTo = new set<int>[htn->sccSize[scc]];

            for (int iTo = 0; iTo < htn->sccSize[scc]; iTo++) {
                int taskTo = htn->sccToTasks[scc][iTo];
                for (int iFrom = 0; iFrom < htn->sccSize[scc]; iFrom++) {
                    int taskFrom = htn->sccToTasks[scc][iFrom];
                    for (int mi = 0; mi < htn->numMethodsForTask[taskFrom]; mi++) {
                        int m = htn->taskToMethods[taskFrom][mi];
                        for (int sti = 0; sti < htn->numSubTasks[m]; sti++) {
                            int subtask = htn->subTasks[m][sti];
                            if (subtask == taskTo) {
                                reachability[iFrom][iTo].insert(m);
                                tasksToFrom[iTo].insert(iFrom);
                                tasksFromTo[iFrom].insert(iTo);
                            }
                        }
                    }
                }
            }

            // copy to permanent data structures
            for (int iF = 0; iF < htn->sccSize[scc]; iF++) {
                //int from = htn->sccToTasks[scc][iF];

                sccInnerFromToMethods[scc][iF] = new int *[htn->sccSize[scc]];
                sccNumInnerFromToMethods[scc][iF] = new int[htn->sccSize[scc]];

                for (int iT = 0; iT < htn->sccSize[scc]; iT++) {
                    //int to = htn->sccToTasks[scc][iT];

                    sccInnerFromTo[scc][iF] = new int[tasksFromTo[iF].size()];
                    sccNumInnerFromTo[scc][iF] = tasksFromTo[iF].size();
                    int l = 0;
                    for (std::set<int>::iterator it = tasksFromTo[iF].begin();
                         it != tasksFromTo[iF].end(); it++) {
                        sccInnerFromTo[scc][iF][l] = *it;
                        l++;
                    }

                    sccInnerToFrom[scc][iT] = new int[tasksToFrom[iT].size()];
                    sccNumInnerToFrom[scc][iT] = tasksToFrom[iT].size();
                    l = 0;
                    for (std::set<int>::iterator it = tasksToFrom[iT].begin();
                         it != tasksToFrom[iT].end(); it++) {
                        sccInnerToFrom[scc][iT][l] = *it;
                        l++;
                    }

                    sccInnerFromToMethods[scc][iF][iT] =
                            new int[reachability[iF][iT].size()];
                    sccNumInnerFromToMethods[scc][iF][iT] =
                            reachability[iF][iT].size();
                    l = 0;
                    for (std::set<int>::iterator it = reachability[iF][iT].begin();
                         it != reachability[iF][iT].end(); it++) {
                        sccInnerFromToMethods[scc][iF][iT][l] = *it;
                        //cout << "reaching " << htn->taskNames[to] << " from " << htn->taskNames[from] << " by " << htn->methodNames[sccInnerFromToMethods[from][to][l]] << endl;
                        l++;
                    }
                }
            }

            for (int iTo = 0; iTo < htn->sccSize[scc]; iTo++) delete[] reachability[iTo];
			delete[] reachability;
			delete[] tasksFromTo;
			delete[] tasksToFrom;
        }

        RlayerCurrent = new int[htn->sccMaxSize];
        RlayerPrev = new int[htn->sccMaxSize];
        Ivars = new int *[htn->sccMaxSize];
        for (int i = 0; i < htn->sccMaxSize; i++) {
            Ivars[i] = new int[htn->sccMaxSize];
        }
        cout << "done." << endl;
    }

    if (this->cNetChange == cNetChangeFull) {
        cout << "  - initializing data structures for net change constraints" << endl;
        vOfL = new int[htn->numStateBits];
        for (int i = 0; i < htn->numVars; i++) {
            for (int j = htn->firstIndex[i]; j <= htn->lastIndex[i]; j++) {
                vOfL[j] = i;
            }
        }

        set<int> *tAP = new set<int>[htn->numStateBits]; // always producer
        set<int> *tSP = new set<int>[htn->numStateBits]; // sometimes producer
        set<int> *tAC = new set<int>[htn->numStateBits]; // always consumer
        for (int a = 0; a < htn->numActions; a++) {
            for (int iE = 0; iE < htn->numAdds[a]; iE++) {
                int eff = htn->addLists[a][iE];

                // try to find other effect that is mutex to eff
                // find var the effect belongs to
                int var = vOfL[eff];
                int prec = -1;
                for (int i = htn->firstIndex[var]; i <= htn->lastIndex[var]; i++) {
                    if (iu.containsInt(htn->precLists[a], 0, htn->numPrecs[a] - 1, i)) {
                        prec = i;
                        break;
                    }
                }
                if (prec == eff)
                    continue;
                if (prec >= 0) {
                    tAP[eff].insert(a);
                    tAC[prec].insert(a);
                    //cout << htn->taskNames[a] << " " << htn->factStrs[prec] << " -> " << htn->factStrs[eff] << endl;
                    continue;
                }

                tSP[eff].insert(a);
            }
        }

        this->numAC = new int[htn->numStateBits];
        this->numAP = new int[htn->numStateBits];
        this->numSP = new int[htn->numStateBits];
        this->acList = new int *[htn->numStateBits];
        this->apList = new int *[htn->numStateBits];
        this->spList = new int *[htn->numStateBits];
        for (int i = 0; i < htn->numStateBits; i++) {
            numAC[i] = tAC[i].size();
            acList[i] = new int[tAC[i].size()];
            int j = 0;
            for (int a : tAC[i]) acList[i][j++] = a;

            numAP[i] = tAP[i].size();
            apList[i] = new int[tAP[i].size()];
            j = 0;
            for (int a : tAP[i]) apList[i][j++] = a;

            numSP[i] = tSP[i].size();
            spList[i] = new int[tSP[i].size()];
            j = 0;
            for (int a : tSP[i]) spList[i][j++] = a;
        }
        delete[] tAC;
        delete[] tAP;
        delete[] tSP;
    }
    if (this->cLmcLms == cLmcLmsFull) {
        hRC = new hhRC2<hsLmCut>(htn, 0, estDISTANCE, false); // todo: which index to use?
    }
    if ((this->cAndOrLms == cAndOrLmsFull) || (this->cAndOrLms == cAndOrLmsOnlyTnI)) {
        causalLMs = new LmCausal(htn);
    }
    printHeuristicInformation();
}

void hhDOfree::printHeuristicInformation() {
    string s = "dof";
    if ((this->cBoolType == IloNumVar::Bool) && (this->cIntType == IloNumVar::Int)) {
        s = s + "IP,";
    } else if ((this->cBoolType == IloNumVar::Float) && (this->cIntType == IloNumVar::Float)) {
        s = s + "LP,";
    } else {
        exit(-1);
    }
    if (this->cTdg == csTdg::cTdgFull) {
        s = s + "TdgFull,";
    } else if (this->cTdg == csTdg::cTdgAllowUC) {
        s = s + "TdgAllowUC,";
    } else if (this->cTdg == csTdg::cTdgNone) {
        s = s + "TdgNone,";
        exit(-1);
    }
    if (this->cPg == csPg::cPgFull) {
        s = s + "PgFull,";
    } else if (this->cPg == csPg::cPgNone) {
        s = s + "PgNone,";
    }
    if (this->cAndOrLms == csAndOrLms::cAndOrLmsFull) {
        s = s + "AndOrLmsFull,";
    } else if (this->cAndOrLms == csAndOrLms::cAndOrLmsOnlyTnI) {
        s = s + "AndOrLmsOnlyTnI,";
    } else if (this->cAndOrLms == csAndOrLms::cAndOrLmsNone) {
        s = s + "AndOrLmsNone,";
    }
    if (this->cLmcLms == csLmcLms::cLmcLmsNone) {
        s = s + "LmcLmsNone,";
    } else if (this->cLmcLms == csLmcLms::cLmcLmsFull) {
        s = s + "LmcLmsFull,";
    }
    if (this->cNetChange == csNetChange::cNetChangeNone) {
        s = s + "NetChangeNone,";
    } else if (this->cNetChange == csNetChange::cNetChangeFull) {
        s = s + "NetChangeFull,";
    }
    if (this->cAddExternalLms == csAddExternalLms::cAddExternalLmsNo) {
        s = s + "AddExternalLmsNo";
    } else if (this->cAddExternalLms == csAddExternalLms::cAddExternalLmsYes) {
        s = s + "AddExternalLmsYes";
    }
    cout << "[HCONF:" << s << "]" << endl;
}

hhDOfree::~hhDOfree() {
    delete[] iUF;
    delete[] iUA;
    delete[] iM;
    delete[] EStartI;
    delete[] iE;
    delete[] EInvSize;
    for (int i = 0; i < htn->numStateBits; i++) {
        delete[] iEInvEffIndex[i];
        delete[] iEInvActionIndex[i];
    }
    delete[] iEInvEffIndex;
    delete[] iEInvActionIndex;
    delete[] iTP;
    delete[] iTA;
    delete pg;
}

void hhDOfree::setHeuristicValue(searchNode *n, searchNode *parent,
                                 int action) {
    int h = this->recreateModel(n);
    n->heuristicValue[index] = h;
    if(n->goalReachable) {
        n->goalReachable = (h != UNREACHABLE);
    }
}

void hhDOfree::setHeuristicValue(searchNode *n, searchNode *parent, int absTask,
                                 int method) {
    this->setHeuristicValue(n, parent, -1);
}

void hhDOfree::updatePG(searchNode *n) {
    // collect preprocessed reachability
    preProReachable.clear();
    for (int i = 0; i < n->numAbstract; i++) {
        for (int j = 0; j < n->unconstraintAbstract[i]->numReachableT; j++) {
            preProReachable.insert(n->unconstraintAbstract[i]->reachableT[j]);
        }
    }
    for (int i = 0; i < n->numPrimitive; i++) {
        for (int j = 0; j < n->unconstraintPrimitive[i]->numReachableT; j++) {
            preProReachable.insert(n->unconstraintPrimitive[i]->reachableT[j]);
        }
    }

    pg->calcReachability(n->state, preProReachable); // calculate reachability
}

int hhDOfree::recreateModel(searchNode *n) {

    if (this->cLmcLms == cLmcLmsFull) {
        int hLMC = this->hRC->setHeuristicValue(n);
        if (hLMC == UNREACHABLE) {
            return UNREACHABLE;
        }
    }

    this->updatePG(n);

    // test for easy special cases
    for (int i = 0; i < htn->gSize; i++) { // make sure goal is reachable (not tested below)
        if (!pg->factReachable(htn->gList[i])) {
            return UNREACHABLE;
        }
    }

    bool tnInonempty = false;
    for (int i = 0; i < n->numContainedTasks; i++) { // is something bottom-up unreachable?
        int t = n->containedTasks[i];
        if (!pg->taskReachable(t)) {
            return UNREACHABLE;
        }
        tnInonempty = true;
    }

    if (!tnInonempty) { // fact-based goal has been checked before -> this is a goal node
        return 0;
    }

    IloEnv lenv;
    IloNumVarArray v(lenv); //  all variables
    IloModel model(lenv);

    int iv = 0;
    // useful facts contains only a subset of all reachable
    for (int i = pg->usefulFactSet.getFirst(); i >= 0;
         i = pg->usefulFactSet.getNext()) {
        v.add(IloNumVar(lenv, 0, 1, cBoolType));
        iUF[i] = iv;
#ifdef NAMEMODEL
        v[iv].setName(("UF" + to_string(i)).c_str());
#endif

        iv++;
    }

    for (int i = pg->reachableTasksSet.getFirst(); i >= 0;
         i = pg->reachableTasksSet.getNext()) {
        v.add(IloNumVar(lenv, 0, INT_MAX, cIntType));
        iUA[i] = iv;
#ifdef NAMEMODEL
        v[iv].setName(("UA" + to_string(i)).c_str());
#endif
        iv++;
    }

    for (int i = pg->reachableMethodsSet.getFirst(); i >= 0;
         i = pg->reachableMethodsSet.getNext()) {
        v.add(IloNumVar(lenv, 0, INT_MAX, cIntType));
        iM[i] = iv;
#ifdef NAMEMODEL
        v[iv].setName(("M" + to_string(i)).c_str());
#endif
        iv++;
    }

    for (int a = pg->reachableTasksSet.getFirst();
         (a >= 0) && (a < htn->numActions); a = pg->reachableTasksSet.getNext()) {
        for (int ai = 0; ai < htn->numAdds[a]; ai++) {
            if (!pg->usefulFactSet.get(htn->addLists[a][ai]))
                continue;
            v.add(IloNumVar(lenv, 0, 1, cBoolType));
            iE[EStartI[a] + ai] = iv;
#ifdef NAMEMODEL
            v[iv].setName(("xE" + to_string(a) + "x" + to_string(ai)).c_str());
#endif
            iv++;
        }
    }

    if ((this->cPg == cPgTimeRelaxed) || (this->cPg == cPgFull)) {
        for (int i = pg->usefulFactSet.getFirst(); i >= 0;
             i = pg->usefulFactSet.getNext()) {
            v.add(IloNumVar(lenv, 0, htn->numActions, cIntType));
            iTP[i] = iv;
#ifdef NAMEMODEL
            v[iv].setName(("TP" + to_string(i)).c_str());
#endif
            iv++;
        }
        for (int a = pg->reachableTasksSet.getFirst();
             (a >= 0) && (a < htn->numActions); a = pg->reachableTasksSet.getNext()) {
            v.add(IloNumVar(lenv, 0, htn->numActions, cIntType));
            iTA[a] = iv;
#ifdef NAMEMODEL
            v[iv].setName(("TA" + to_string(a)).c_str());
#endif
            iv++;
        }
    }

    // create main optimization function
    IloNumExpr mainExp(lenv);
    int costs = 1; // unit costs
    for (int i = pg->reachableTasksSet.getFirst();
         (i >= 0) && (i < htn->numActions); i = pg->reachableTasksSet.getNext()) {
        if (this->cSetting == cOptimal) { // if satisficing: use unit-costs, aka modification distance, else use action costs
            costs = htn->actionCosts[i];
        }
        mainExp = mainExp + (costs * v[iUA[i]]);
    }

    if (this->cSetting == cSatisficing) { // add other modifications, i.e., methods
        costs = 1;
        for (int i = pg->reachableMethodsSet.getFirst(); i >= 0;
            i = pg->reachableMethodsSet.getNext()) {
            mainExp = mainExp + (costs * v[iM[i]]);
        }
    }

    model.add(IloMinimize(lenv, mainExp));

    if ((this->cPg == cPgTimeRelaxed) || (this->cPg == cPgFull)) {
        // C1
        for (int i = 0; i < htn->gSize; i++) {
            if (pg->usefulFactSet.get(htn->gList[i]))
                model.add(v[iUF[htn->gList[i]]] == 1);
        }

        for (int a = pg->reachableTasksSet.getFirst();
             (a >= 0) && (a < htn->numActions); a =
                                                        pg->reachableTasksSet.getNext()) {
            // C2
            for (int i = 0; i < htn->numPrecs[a]; i++) {
                int prec = htn->precLists[a][i]; // precs are always useful
                if (pg->usefulFactSet.get(prec))
                    model.add(100 * v[iUF[prec]] >= v[iUA[a]]); // TODO: how large must the const be?
            }
            // C3
            for (int iAdd = 0; iAdd < htn->numAdds[a]; iAdd++) {
                int fAdd = htn->addLists[a][iAdd];
                if (pg->usefulFactSet.get(fAdd))
                    model.add(v[iUA[a]] - v[iE[EStartI[a] + iAdd]] >= 0);
            }
        }

        // C4
        for (int f = pg->usefulFactSet.getFirst(); f >= 0;
             f = pg->usefulFactSet.getNext()) {
            IloNumExpr c4(lenv);

            for (int i = 0; i < EInvSize[f]; i++) {
                int a = iEInvActionIndex[f][i];
                int iAdd = iEInvEffIndex[f][i];
                if (pg->taskReachable(a))
                    c4 = c4 + v[iE[EStartI[a] + iAdd]];
            }
            model.add(c4 - v[iUF[f]] == 0);
        }

        if (this->cPg == cPgFull) {
            // C5
            for (int a = pg->reachableTasksSet.getFirst();
                 (a >= 0) && (a < htn->numActions); a =
                                                            pg->reachableTasksSet.getNext()) {
                if (htn->numPrecs[a] == 0) {
                    continue;
                }
                for (int i = 0; i < htn->numPrecs[a]; i++) {
                    int prec = htn->precLists[a][i];
                    if (pg->usefulFactSet.get(prec))
                        model.add(v[iTA[a]] - v[iTP[prec]] >= 0);
                }
            }

            // C6
            for (int a = pg->reachableTasksSet.getFirst();
                 (a >= 0) && (a < htn->numActions); a =
                                                            pg->reachableTasksSet.getNext()) {
                if (htn->numAdds[a] == 0)
                    continue;
                for (int iadd = 0; iadd < htn->numAdds[a]; iadd++) {
                    int add = htn->addLists[a][iadd];
                    if (!pg->usefulFactSet.get(add))
                        continue;
                    model.add(
                            v[iTA[a]] + 1
                            <= v[iTP[add]]
                               + (htn->numActions + 1)
                                 * (1 - v[iE[EStartI[a] + iadd]]));
                }
            }
        }
    }

    // HTN stuff
    std::vector<IloExpr> tup(htn->numTasks, IloExpr(lenv)); // every task has been produced by some method or been in tnI

    for (int m = pg->reachableMethodsSet.getFirst(); m >= 0;
         m = pg->reachableMethodsSet.getNext()) {
        for (int iST = 0; iST < htn->numSubTasks[m]; iST++) {
            int subtask = htn->subTasks[m][iST];
            tup[subtask] = tup[subtask] + v[iM[m]];
        }
    }

    for (int i = pg->reachableTasksSet.getFirst(); i >= 0;
         i = pg->reachableTasksSet.getNext()) {
        int iOfT = iu.indexOf(n->containedTasks, 0, n->numContainedTasks - 1, i);
        if (iOfT >= 0) {
            model.add(tup[i] + n->containedTaskCount[iOfT] == v[iUA[i]]);
        } else {
            model.add(tup[i] + 0 == v[iUA[i]]);
        }
        //model.add(tup[i] + n->TNIcount[i] == v[iUA[i]]); // set directly
        if (i >= htn->numActions) {
            IloExpr x(lenv);
            for (int iMeth = 0; iMeth < htn->numMethodsForTask[i]; iMeth++) {
                int m = htn->taskToMethods[i][iMeth];
                if (pg->methodReachable(m)) {
                    x = x + v[iM[m]];
                }
            }
            model.add(x == v[iUA[i]]);
        }
    }

    if (this->cTdg == cTdgFull) { // prevent disconnected cycles
        for (int iSCC = 0; iSCC < htn->numCyclicSccs; iSCC++) {
            int scc = htn->sccsCyclic[iSCC];

            // initial layer R0
            // name schema: R-SCC-Time-Task
            for (int iT = 0; iT < htn->sccSize[scc]; iT++) {
                int task = htn->sccToTasks[scc][iT];
                if (!pg->taskReachable(task))
                    continue;
                v.add(IloNumVar(lenv, 0, INT_MAX, cIntType));
#ifdef NAMEMODEL
                v[iv].setName(
                        ("R_" + to_string(scc) + "_0_" + to_string(iT)).c_str());
#endif
                RlayerCurrent[iT] = iv++;
            }

            // C9
            for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) {
                int task = htn->sccToTasks[scc][iTask];
                if (!pg->taskReachable(task))
                    continue;
                IloExpr c9(lenv);
                int iOfT = iu.indexOf(n->containedTasks, 0, n->numContainedTasks - 1, task);
                if (iOfT >= 0) {
                    c9 = c9 + n->containedTaskCount[iOfT];
                }
                for (int iMeth = 0; iMeth < sccNumIncommingMethods[scc][iTask];
                     iMeth++) {
                    int m = sccIncommingMethods[scc][iTask][iMeth];
                    if (pg->methodReachable(m))
                        c9 = c9 + v[iM[m]];
                }
                model.add(c9 >= v[RlayerCurrent[iTask]]);
            }

            for (int iTime = 1; iTime < htn->sccSize[scc]; iTime++) {
                int *Rtemp = RlayerPrev;
                RlayerPrev = RlayerCurrent;
                RlayerCurrent = Rtemp;

                // create variables for ILP (needs to be done first, there may be forward references)
                for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) {
                    int task = htn->sccToTasks[scc][iTask];
                    if (!pg->taskReachable(task))
                        continue;
                    v.add(IloNumVar(lenv, 0, INT_MAX, cIntType));
#ifdef NAMEMODEL
                    v[iv].setName(
                            ("R_" + to_string(scc) + "_" + to_string(iTime) + "_"
                             + to_string(iTask)).c_str());
#endif
                    RlayerCurrent[iTask] = iv++;
                    for (int iTask2 = 0; iTask2 < htn->sccSize[scc]; iTask2++) {
                        int task2 = htn->sccToTasks[scc][iTask2];
                        if (!pg->taskReachable(task2))
                            continue;
                        v.add(IloNumVar(lenv, 0, INT_MAX, cIntType));
#ifdef NAMEMODEL
                        v[iv].setName(
                                ("I_" + to_string(scc) + "_" + to_string(iTime)
                                 + "_" + to_string(iTask) + "_"
                                 + to_string(iTask2)).c_str());
#endif
                        Ivars[iTask][iTask2] = iv++;
                    }
                }

                // create constraints C10, C11 and C12
                for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) { // t in the paper
                    // C10
                    int task = htn->sccToTasks[scc][iTask];
                    if (!pg->taskReachable(task))
                        continue;
                    IloExpr c10(lenv);
                    c10 = c10 + v[RlayerPrev[iTask]];

                    for (int k = 0; k < sccNumInnerToFrom[scc][iTask]; k++) {
                        int iTask2 = sccInnerToFrom[scc][iTask][k];
                        int task2 = htn->sccToTasks[scc][iTask2];
                        if (!pg->taskReachable(task2))
                            continue;
                        c10 = c10 + v[Ivars[iTask2][iTask]];
                    }
                    model.add(v[RlayerCurrent[iTask]] <= c10);

                    for (int iTask2 = 0; iTask2 < htn->sccSize[scc]; iTask2++) { // t' in the paper
                        int task2 = htn->sccToTasks[scc][iTask2];
                        if (!pg->taskReachable(task2))
                            continue;

                        // C11
                        model.add(v[Ivars[iTask2][iTask]] <= v[RlayerPrev[iTask2]]);

                        // C12
                        IloExpr c12(lenv);
                        c12 = c12 + 0;
                        for (int k = 0;
                             k < sccNumInnerFromToMethods[scc][iTask2][iTask];
                             k++) {
                            int m = sccInnerFromToMethods[scc][iTask2][iTask][k];
                            if (!pg->methodReachable(m))
                                continue;
                            c12 = c12 + v[iM[m]];
                        }
                        model.add(v[Ivars[iTask2][iTask]] <= c12);
                    }
                }
            }

            // C13
            for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) {
                int task = htn->sccToTasks[scc][iTask];
                if (!pg->taskReachable(task))
                    continue;
                model.add(v[iUA[task]] <= v[RlayerCurrent[iTask]] * 100); // todo: what is a sufficiently high number?
            }
        }
    } // end of prevent disconnected cycles

    if (this->cAddExternalLms == cAddExternalLmsYes) {
#ifdef TRACKLMSFULL
        for(int iLM = 0; iLM < n->numtLMs; iLM ++) {
                int lm = n->tLMs[iLM];
                model.add(v[iUA[lm]] >= 1);
            }
            for(int iLM = 0; iLM < n->nummLMs; iLM ++) {
                int lm = n->mLMs[iLM];
                model.add(v[iM[lm]] >= 1);
            }
            for(int iLM = 0; iLM < n->numfLMs; iLM ++) {
                int lm = n->fLMs[iLM];
                model.add(v[iUF[lm]] >= 1);
            }
#endif
    }

    if (this->cLmcLms == cLmcLmsFull) {
        // add lm cut landmarks
        for (LMCutLandmark *storedcut : *this->hRC->cuts) {
            IloExpr lm(lenv);
            for (int i = 0; i < storedcut->size; i++) {
                if (storedcut->isAction(i)) {
                    int a = storedcut->lm[i];
                    if (!pg->taskReachable(a))
                        continue;
                    lm = lm + v[iUA[a]];
                } else {
                    int m = storedcut->lm[i];
                    if (!pg->methodReachable(m))
                        continue;
                    lm = lm + v[iM[m]];
                }
            }
            model.add(lm >= 1);
        }
    }

    if ((this->cAndOrLms == cAndOrLmsOnlyTnI) || (this->cAndOrLms == cAndOrLmsFull)) {
        causalLMs->calcLMs(n, pg);
        landmark **andOrLMs = causalLMs->getLMs();
        for (int i = 0; i < causalLMs->getNumLMs(); i++) {
            landmark *lm = andOrLMs[i];
            if (lm->type == fact) {
                int f = lm->lm[0];
                // careful: only add when not contained in initial state
                if ((lm->connection == atom) && (!n->state[f]) && (pg->usefulFactSet.get(f))) {
                    model.add(v[iUF[f]] >= 1);
                }
            } else if (lm->type == METHOD) {
                int mlm = lm->lm[0];
                if ((lm->connection == atom) && (pg->methodReachable(mlm))) {
                    model.add(v[iM[mlm]] >= 1);
                }
            } else {
                assert(lm->type == task);
                if (lm->connection == atom) {
                    int tlm = lm->lm[0];
                    if (pg->taskReachable(tlm))
                        model.add(v[iUA[tlm]] >= 1);
                }
            }
        }
    }
    if (this->cAndOrLms == cAndOrLmsFull) {// add all implications
        for (int t = pg->reachableTasksSet.getFirst(); t >= 0; t = pg->reachableTasksSet.getNext()) {
            causalLMs->initIterTask(t);
            while (causalLMs->iterHasNext()) {
                int lm = causalLMs->iterGetLm();
                lmType type = causalLMs->iterGetLmType();
                if (type == task) {
                    if (pg->taskReachable(lm)) {
                        model.add(v[iUA[lm]] * largeC >= v[iUA[t]]);
                    } else {
                        model.add(v[iUA[t]] == 0);
                    }
                } else if (type == fact) {
                    if ((!n->state[lm]) && (pg->usefulFactSet.get(lm))) {
                        model.add(v[iUF[lm]] * largeC >= v[iUA[t]]);
                    } else if (!pg->factReachable(lm)) {
                        model.add(v[iUA[t]] == 0);
                    }
                } else {
                    assert(type == METHOD);
                    if (pg->methodReachable(lm)) {
                        model.add(v[iM[lm]] * largeC >= v[iUA[t]]);
                    } else {
                        model.add(v[iUA[t]] == 0);
                    }
                }
                causalLMs->iterate();
            }
        }
        for (int m = pg->reachableMethodsSet.getFirst(); m >= 0; m = pg->reachableMethodsSet.getNext()) {
            causalLMs->initIterMethod(m);
            while (causalLMs->iterHasNext()) {
                int lm = causalLMs->iterGetLm();
                lmType type = causalLMs->iterGetLmType();
                if (type == task) {
                    if (pg->taskReachable(lm)) {
                        model.add(v[iUA[lm]] * largeC >= v[iM[m]]);
                    } else {
                        model.add(v[iM[m]] == 0);
                    }
                } else if (type == fact) {
                    if ((!n->state[lm]) && (pg->usefulFactSet.get(lm))) {
                        model.add(v[iUF[lm]] * largeC >= v[iM[m]]);
                    } else if (!pg->factReachable(lm)) {
                        model.add(v[iM[m]] == 0);
                    }
                } else {
                    assert(type == METHOD);
                    if (pg->methodReachable(lm)) {
                        model.add(v[iM[lm]] * largeC >= v[iM[m]]);
                    } else {
                        model.add(v[iM[m]] == 0);
                    }
                }
                causalLMs->iterate();
            }
        }
        for (int f = pg->usefulFactSet.getFirst(); f >= 0; f = pg->usefulFactSet.getNext()) {
            causalLMs->initIterFact(f);
            while (causalLMs->iterHasNext()) {
                int lm = causalLMs->iterGetLm();
                lmType type = causalLMs->iterGetLmType();
                if (type == task) {
                    if (pg->taskReachable(lm)) {
                        model.add(v[iUA[lm]] * largeC >= v[iUF[f]]);
                    } else {
                        model.add(v[iUF[f]] == 0);
                    }
                } else if (type == fact) {
                    if ((!n->state[lm]) && (pg->usefulFactSet.get(lm))) {
                        model.add(v[iUF[lm]] * largeC >= v[iUF[f]]);
                    } else if (!pg->factReachable(lm)) {
                        model.add(v[iUF[f]] == 0);
                    }
                } else {
                    assert(type == METHOD);
                    if (pg->methodReachable(lm)) {
                        model.add(v[iM[lm]] * largeC >= v[iUF[f]]);
                    } else {
                        model.add(v[iUF[f]] == 0);
                    }
                }
                causalLMs->iterate();
            }
        }
    }

    if (this->cNetChange == cNetChangeFull) {
        for (int f = 0; f < htn->numStateBits; f++) {
            if (!pg->usefulFactSet.get(f))
                continue;

            // goal stuff
            bool vEqVInG = iu.containsInt(htn->gList, 0, htn->gSize - 1, f);
            bool ovEqVInG = false; // variable where v belongs to is set to other value in G
            if (!vEqVInG) {
                int var = vOfL[f];
                for (int i = htn->firstIndex[var]; i < htn->lastIndex[var]; i++) {
                    if (iu.containsInt(htn->gList, 0, htn->gSize - 1, i)) {
                        ovEqVInG = true;
                        break;
                    }
                }
            }
            bool VnotInG = (!vEqVInG && !ovEqVInG); // the entire variable v belongs to is not contained in G
            int l;
            if (n->state[f]) {
                if ((VnotInG) || (ovEqVInG)) {
                    l = -1;
                } else {
                    l = 0;
                }
            } else { // v not true in s
                if (vEqVInG) {
                    l = 1;
                } else {
                    l = 0;
                }
            }
            IloExpr ncl(lenv);
            for (int ia = 0; ia < this->numAP[f]; ia++) {
                int a = apList[f][ia];
                if (!pg->taskReachable(a)) continue;
                ncl = ncl + v[iUA[a]];
            }
            for (int ia = 0; ia < this->numSP[f]; ia++) {
                int a = spList[f][ia];
                if (!pg->taskReachable(a)) continue;
                ncl = ncl + v[iUA[a]];
            }
            for (int ia = 0; ia < this->numAC[f]; ia++) {
                int a = acList[f][ia];
                if (!pg->taskReachable(a)) continue;
                ncl = ncl - v[iUA[a]];
            }
            model.add(ncl >= l);
        }
    }

    IloCplex cplex(model);

    cplex.setParam(IloCplex::Param::Threads, 1);
    //cplex.setParam(IloCplex::Param::TimeLimit, time / CHECKAFTER); // for some time
    //cplex.setParam(IloCplex::MIPEmphasis, CPX_MIPEMPHASIS_FEASIBILITY); // focus on feasability
    //cplex.setParam(IloCplex::MIPEmphasis, IloCplex::MIPEmphasisType::MIPEmphasisFeasibility); // focus on feasability
    //cplex.setParam(IloCplex::IntSolLim, 1); // stop after first integer solution

    cplex.setOut(lenv.getNullStream());
    cplex.setWarning(lenv.getNullStream());

    int res = -1;
    if (cplex.solve()) {
        // todo: use ceil for LP
        res = round(cplex.getObjValue());
        //IloCplex::CplexStatus status = cplex.getCplexStatus();
        //cout << endl << endl << "Status: " << status << endl;

        //cout << "RCLMC " << hLMC << " DOF " << res << endl;
        /*} else if (cplex.getStatus() == IloAlgorithm::Status::Unknown) {
         cout << "value: time-limit" << endl;
         res = 0;*/

        /*
        cout << endl << endl << "SUM: " << res << endl;
        for (int i = pg->reachableTasksSet.getFirst(); (i >= 0) && (i < htn->numActions); i = pg->reachableTasksSet.getNext()) {
            double d = cplex.getValue(v[iUA[i]]);
            if (d > 0.000001) {
                cout <<  "T " << htn->taskNames[i] << " == " << d << endl;
            }
        }

        for (int i = pg->reachableMethodsSet.getFirst(); i >= 0;
             i = pg->reachableMethodsSet.getNext()) {
            double d = cplex.getValue(v[iM[i]]);
            if (d > 0.000001) {
                cout <<  "M " << htn->taskNames[i] << " == " << d << endl;
            }
        }
        */
    } else {
        res = UNREACHABLE;
/*
        string dateiname = "/home/dh/Schreibtisch/temp2/model-";
        dateiname += fileID;
        fileID++;
        cplex.exportModel(dateiname.c_str());
*/
    }

    lenv.end();
    return res;
}
