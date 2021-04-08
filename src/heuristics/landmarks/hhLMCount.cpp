/*
 * hhLMCount.cpp
 *
 *  Created on: 08.01.2020
 *      Author: dh
 */

#include <stdlib.h>
#include <cassert>
#include <map>
#include <landmarks/lmExtraction/LmCausal.h>
#include "hhLMCount.h"
#include "lmExtraction/LMsInAndOrGraphs.h"
#include "lmExtraction/LmFdConnector.h"
#include "lmDataStructures/LmMap.h"

namespace progression {
//
//void hhLMCount::deleteFulfilledLMs(searchNode *tnI){
//	set<int> deleteLMs;
//	for(int i = 0; i < tnI->numLMs; i++) {
//		landmark* lm = tnI->lms[i];
//		if(lm->type == fact) {
//			if((lm->connection == atom) || (lm->connection == conjunctive)){
//				bool fulfilled = true;
//				for(int j = 0; j < lm->size; j++) {
//					if(!tnI->state[lm->lm[j]]) {
//						fulfilled = false;
//						break;
//					}
//				}
//				if(fulfilled)
//					deleteLMs.insert(i);
//			} else {
//				for(int j = 0; j < lm->size; j++) {
//					if(tnI->state[lm->lm[j]]) {
//						deleteLMs.insert(i);
//						break;
//					}
//				}
//			}
//		} else if(lm->type == task) {
//			if((lm->connection == atom) || (lm->connection == conjunctive)){
//				bool fulfilled = true;
//				for(int j = 0; j < lm->size; j++) {
//					if(!iu.containsInt(tnI->containedTasks, 0, tnI->numContainedTasks - 1, lm->lm[j])) {
//						fulfilled = false;
//						break;
//					}
//				}
//				if(fulfilled)
//					deleteLMs.insert(i);
//			} else {
//				for(int j = 0; j < lm->size; j++) {
//					if(!iu.containsInt(tnI->containedTasks, 0, tnI->numContainedTasks - 1, lm->lm[j])) {
//						deleteLMs.insert(i);
//						break;
//					}
//				}
//			}
//		}
//	}
//
//	if(deleteLMs.size() > 0) {
//		cout << "The landmarks ";
//		for(int lm : deleteLMs){
//			cout << lm << " ";
//		}
//		cout << "are already fulfilled in tnI and are deleted." << endl;
//	}
//
//	int size = tnI->numLMs - deleteLMs.size();
//	landmark** lms2 = new landmark*[size];
//
//	int j = 0;
//	for(int i = 0; i < tnI->numLMs; i++) {
//		if(deleteLMs.find(i) == deleteLMs.end()) {
//			lms2[j++] = tnI->lms[i];
//		}
//	}
//	assert(j == size);
//	tnI->numLMs = size;
//	delete[] tnI->lms;
//	tnI->lms = lms2;
//}

hhLMCount::hhLMCount(Model* htn, int index, searchNode *tnI, lmFactory typeOfFactory) : Heuristic(htn, index){
	this->m = htn;
	map<int, set<int>*> luT;
	map<int, set<int>*> luM;
	map<int, set<int>*> luF;

	cout << "LM count heuristic" << endl;
	if (typeOfFactory == lmfLOCAL) {
		cout << "- using recursive local landmarks" << endl;
        LMsInAndOrGraphs lms (htn);
		lms.generateLocalLMs(htn, tnI);
//		tnI->numLMs = lms.getNumLMs();
//		tnI->lms = lms.getLMs();
	} else if(typeOfFactory == lmfANDOR) {
		cout << "- using AND/OR graph landmark extraction" << endl;
        LmCausal lms (htn);
        lms.calcLMs(tnI);
//		tnI->numLMs = lms.getNumLMs();
//		tnI->lms = lms.getLMs();
	} else if(typeOfFactory == lmfFD) {
		cout << "- using FD landmark extraction" << endl;
		LmFdConnector FDcon;
		FDcon.createLMs(htn);
//		tnI->numLMs = FDcon.getNumLMs();
//		tnI->lms = FDcon.getLMs();
	} else {
		cout << "ERROR: Unknown landmark extraction method" << endl;
		exit(-1);
	}

	//prettyPrintLMs(htn,tnI);
//	this->deleteFulfilledLMs(tnI);

//	tnI->lookForT = createElemToLmMapping(tnI, task);
//	tnI->lookForF = createElemToLmMapping(tnI, fact);
//	tnI->lookForM = createElemToLmMapping(tnI, METHOD);
}

    string hhLMCount::getDescription() {
        return "LM-Count";
    }

    void hhLMCount::setHeuristicValue(searchNode *n, searchNode *parent, int action) {

    }

    void hhLMCount::setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) {

    }

    hhLMCount::~hhLMCount() {

    }
//
//
//lookUpTab* hhLMCount::createElemToLmMapping(searchNode *n, lmType type){
//	map<int, set<int>*> tMap;
//	for(int iLM = 0; iLM < n->numLMs; iLM++) {
//		landmark* lm = n->lms[iLM];
//		if(lm->type == type) {
//			for(int i = 0; i < lm->size; i++) {
//				int e = lm->lm[i];
//				if(tMap.find(e) == tMap.end())
//					tMap.insert(make_pair(e, new set<int>));
//				tMap[e]->insert(iLM); // element is contained in LM number i
//			}
//		}
//	}
//
//	lookUpTab* res = new lookUpTab(tMap.size());
//	int i = 0;
//	for(map<int, set<int>*>::iterator it = tMap.begin(); it != tMap.end(); ++it) {
//		int key = it->first;
//		set<int>* val = it->second;
//		res->lookFor[i] = new LmMap(key, val->size());
//		int j = 0;
//		for(int v : *val)
//			res->lookFor[i]->containedInLMs[j++] = v;
//		delete val;
//		i++;
//	}
//	return res;
//}
//
//
//void hhLMCount::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
//	setHeuristicValue(n);
//}
//
//void hhLMCount::setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) {
//	setHeuristicValue(n);
//}
//
//void hhLMCount::setHeuristicValue(searchNode *n) {
//
//	n->heuristicValue = n->numLMs - (n->reachedfLMs + n->reachedmLMs + n->reachedtLMs);
//	//n->heuristicValue = (n->numfLMs + n->numtLMs + n->nummLMs);
//
//#ifdef LMCANDORRA
//	preProReachable.clear();
//	for (int i = 0; i < n->numAbstract; i++) {
//		for (int j = 0; j < n->unconstraintAbstract[i]->numReachableT; j++) {
//			preProReachable.insert(n->unconstraintAbstract[i]->reachableT[j]);
//		}
//	}
//	for (int i = 0; i < n->numPrimitive; i++) {
//		for (int j = 0; j < n->unconstraintPrimitive[i]->numReachableT; j++) {
//			preProReachable.insert(n->unconstraintPrimitive[i]->reachableT[j]);
//		}
//	}
//
//	pg->calcReachability(n->state, preProReachable); // calculate reachability
//
//	bool gReachable = true;
//	for(int i = 0; i < n->numtLMs; i++) {
//		int tlm = n->tLMs[i];
//		if(!pg->reachableTasksSet.get(tlm)) {
//			gReachable = false;
//			break;
//		}
//	}
//	if(gReachable){
//		for (int i = 0; i< n->nummLMs; i++) {
//			int mlm = n->mLMs[i];
//			if(!pg->reachableMethodsSet.get(mlm)) {
//				gReachable = false;
//				break;
//			}
//		}
//	}
//	if(gReachable){
//		for (int i = 0; i< n->numfLMs; i++) {
//			int flm = n->fLMs[i];
//			if(!pg->factReachable(flm)) {
//				gReachable = false;
//				break;
//			}
//		}
//	}
//	if (!gReachable) {
//		pruned++;
//	}
//	n->goalReachable = gReachable;
//#endif
//}
//
//hhLMCount::~hhLMCount() {
//#ifdef LMCANDORRA
//	cout << "- heuristic pruned " << this->pruned << " nodes" << endl;
//#endif
//	// TODO Auto-generated destructor stub
//}
//
//void hhLMCount::prettyPrintLMs(Model* htn, searchNode *n) {
//	for(int i = 0; i < n->numLMs; i++) {
//		landmark* lm = n->lms[i];
//		cout << "- LM ";
//		string* nameStrs;
//		if(lm->type == fact){
//			cout << "fact";
//			nameStrs = htn->factStrs;
//		} else if(lm->type == task){
//			cout << "task";
//			nameStrs = htn->taskNames;
//		} else if(lm->type == METHOD){
//			cout << "meth";
//			nameStrs = htn->methodNames;
//		}
//		cout << " ";
//
//		if(lm->connection == atom){
//			cout << "atom";
//		} else if(lm->connection == conjunctive){
//			cout << "conj";
//		} else if(lm->connection == disjunctive){
//			cout << "disj";
//		}
//		for(int j = 0; j < lm->size; j++) {
//			cout << " " << nameStrs[lm->lm[j]];
//		}
//		cout << endl;
//	}
//}

} /* namespace progression */
