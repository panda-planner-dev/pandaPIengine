//
// Created by dh on 01.04.20.
//

#include "hhStatisticsCollector.h"
//
//hhStatisticsCollector::hhStatisticsCollector(Model *htn, searchNode *n, int depth) {
//    this->maxDepth = depth;
//    this->m = htn;
//
//    int index = 1;
//
//    this->hRcLmc = new hhRC2<hsLmCut>(htn, index, false);
//    for (int ilp = 0; ilp <= 1; ilp++) {
//        for (int setting = cSatisficing; setting <= cOptimal; setting++) {
//            for (int tdg = cTdgFull; tdg <= cTdgAllowUC; tdg++) {
//                for (int pg = cPgFull; pg <= cPgNone; pg++) {
//                    for (int aolms = cAndOrLmsNone; aolms <= cAndOrLmsFull; aolms++) {
//                        for (int lmclms = cLmcLmsFull; lmclms <= cLmcLmsNone; lmclms++) {
//                            for (int nc = cNetChangeFull; nc <= cNetChangeNone; nc++) {
//                                csSetting eSetting = static_cast<csSetting>(setting);
//                                csTdg eTDG = static_cast<csTdg>(tdg);
//                                csPg ePG = static_cast<csPg>(pg);
//                                csAndOrLms eAOLMs = static_cast<csAndOrLms>(aolms);
//                                csLmcLms eLMCLMs = static_cast<csLmcLms>(lmclms);
//                                csNetChange eNC = static_cast<csNetChange>(nc);
//
//                                // Skip the ones not needed here
//                                if (ePG == cPgTimeRelaxed)
//                                    continue;
//                                if (eSetting == cSatisficing)
//                                    continue;
//
//                                hhDOfree *h;
//                                if (ilp == 0) {
//                                    h = new hhDOfree(htn, n, index, IloNumVar::Int, IloNumVar::Bool,
//                                                     eSetting, eTDG, ePG, eAOLMs, eLMCLMs, eNC, cAddExternalLmsNo);
//                                } else {
//                                    h = new hhDOfree(htn, n, index, IloNumVar::Float, IloNumVar::Float,
//                                                     eSetting, eTDG, ePG, eAOLMs, eLMCLMs, eNC, cAddExternalLmsNo);
//                                }
//                                this->ilpHs.push_back(h);
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }
//    /*hhDOfree *h = new hhDOfree(htn, n, IloNumVar::Int, IloNumVar::Bool, cTdgAllowUC, cPgNone, cAndOrLmsNone, cLmcLmsNone, cNetChangeNone, cAddExternalLmsNo);
//    hhDOfree *h = new hhDOfree(htn, n, IloNumVar::Int, IloNumVar::Bool, cTdgFull, cPgFull, cAndOrLmsNone, cLmcLmsNone, cNetChangeNone, cAddExternalLmsNo);
//    this->ilpHs.push_back(h);
//    hhDOfree *h2 = new hhDOfree(htn, n, IloNumVar::Int, IloNumVar::Bool, cTdgFull, cPgFull, cAndOrLmsNone, cLmcLmsFull, cNetChangeNone, cAddExternalLmsNo);
//    this->ilpHs.push_back(h2);*/
//
//    cout << "0 [RC,LMC]" << endl;
//    for (int i = 0; i < ilpHs.size(); i++) {
//        cout << (i + 1) << " ";
//        ilpHs[i]->printHeuristicInformation();
//    }
//    cout << "Generated " << ilpHs.size() << " configurations" << endl;
//}
//
//void hhStatisticsCollector::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
//    this->setHeuristicValue(n);
//}
//
//void hhStatisticsCollector::setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) {
//    this->setHeuristicValue(n);
//}
//
//void hhStatisticsCollector::setHeuristicValue(searchNode *n) {
//    //m->writeTDGCompressed("/home/dh/Schreibtisch/analysis/tdg.dot" ,n);
//    //m->writeTDG("/home/dh/Schreibtisch/analysis/tdg2.dot");
//    cout << "[hinf," << m->filename << "," << n->modificationDepth << ",";
//    int hLMC = hRcLmc->setHeuristicValue(n);
//    if (hLMC == UNREACHABLE) {
//        cout << "-1";
//    } else {
//        cout << hLMC;
//    }
//    for (int i = 0; i < ilpHs.size(); i++) {
//        cout << ",";
//        ilpHs[i]->setHeuristicValue(n, nullptr, -1);
//        if (!n->goalReachable) {
//            cout << "-1";
//        } else {
//            cout << n->heuristicValue;
//        }
//    }
//    cout << "]" << endl;
//    if (n->modificationDepth >= this->maxDepth) {
//        n->goalReachable = false;
//    } else {
//        n->heuristicValue[0] = n->modificationDepth;
//    }
//}
