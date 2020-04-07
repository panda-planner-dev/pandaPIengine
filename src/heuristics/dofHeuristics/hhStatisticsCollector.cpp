//
// Created by dh on 01.04.20.
//

#include "hhStatisticsCollector.h"

hhStatisticsCollector::hhStatisticsCollector(Model *htn, searchNode *n, int depth) {
    this->maxDepth = depth;
    this->m = htn;

    this->hRcLmc = new hhRC2(htn);
    for (int ilp = 0; ilp <= 1; ilp++) {
        for (int tdg = cTdgFull; tdg <= cTdgAllowUC; tdg++) {
            for (int pg = cPgFull; pg <= cPgNone; pg++) {
                for (int aolms = cAndOrLmsOnlyTnI; aolms <= cAndOrLmsNone; aolms++) {
                    for (int lmclms = cLmcLmsFull; lmclms <= cLmcLmsNone; lmclms++) {
                        for (int nc = cNetChangeFull; nc <= cNetChangeNone; nc++) {
                            csTdg eTDG = static_cast<csTdg>(tdg);
                            csPg ePG = static_cast<csPg>(pg);
                            csAndOrLms eAOLMs = static_cast<csAndOrLms>(aolms);
                            csLmcLms eLMCLMs = static_cast<csLmcLms>(lmclms);
                            csNetChange eNC = static_cast<csNetChange>(nc);

                            if (eAOLMs == csAndOrLms::cAndOrLmsFull) // skip landmark implications
                                continue;
                            if (ePG == cPgTimeRelaxed) // Skip PG time relaxation
                                continue;

                            hhDOfree *h;
                            if (ilp == 0) {
                                h = new hhDOfree(htn, n, IloNumVar::Int, IloNumVar::Bool,
                                                 eTDG, ePG, eAOLMs, eLMCLMs, eNC, cAddExternalLmsNo);
                            } else {
                                h = new hhDOfree(htn, n, IloNumVar::Float, IloNumVar::Float,
                                                 eTDG, ePG, eAOLMs, eLMCLMs, eNC, cAddExternalLmsNo);
                            }
                            this->ilpHs.push_back(h);
                        }
                    }
                }
            }
        }
    }
    //hhDOfree *h = new hhDOfree(htn, n, IloNumVar::Int, IloNumVar::Bool, cTdgAllowUC, cPgNone, cAndOrLmsNone, cLmcLmsNone, cNetChangeNone, cAddExternalLmsNo);
    //this->ilpHs.push_back(h);

    cout << "0 [RC,LMC]" << endl;
    for (int i = 0; i < ilpHs.size(); i++) {
        cout << (i + 1) << " ";
        ilpHs[i]->printHeuristicInformation();
    }
    cout << "Generated " << ilpHs.size() << " configurations" << endl;
}

void hhStatisticsCollector::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
    this->setHeuristicValue(n);
}

void hhStatisticsCollector::setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) {
    this->setHeuristicValue(n);
}

void hhStatisticsCollector::setHeuristicValue(searchNode *n) {
    cout << "[hinf," << m->filename << "," << n->modificationDepth << ",";
    int hLMC = hRcLmc->setHeuristicValue(n);
    if (hLMC == UNREACHABLE) {
        cout << "-1";
    } else {
        cout << hLMC;
    }
    for (int i = 0; i < ilpHs.size(); i++) {
        cout << ",";
        ilpHs[i]->setHeuristicValue(n, nullptr, -1);
        if (!n->goalReachable) {
            cout << "-1";
        } else {
            cout << n->heuristicValue;
        }
    }
    cout << "]" << endl;
    if (n->modificationDepth >= this->maxDepth) {
        n->goalReachable = false;
    } else {
        n->heuristicValue = n->modificationDepth;
    }
}
