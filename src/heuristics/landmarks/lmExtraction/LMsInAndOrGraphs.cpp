/*
 * LMsInAndOrGraphs.cpp
 *
 *  Created on: 30.11.2019
 *      Author: Daniel Höller
 */

#include <vector>
#include <list>
#include <set>
#include <cassert>
#include <sys/time.h>
#include "LMsInAndOrGraphs.h"

namespace progression {

LMsInAndOrGraphs::LMsInAndOrGraphs(Model* htn) {
	bool loopback = false;
	this->htn = htn;

	if(!loopback)
		numNodes = htn->numStateBits + htn->numTasks + htn->numMethods;
	else
		numNodes = htn->numStateBits + htn->numTasks + htn->numMethods + htn->numActions;

	N = new vector<int> [numNodes];
	Ninv = new vector<int> [numNodes];
	nodeType = new int[numNodes];

	for(int i = 0; i < htn->numStateBits; i++)
		nodeType[fNode(i)] = tOR;
	for(int i = 0; i < htn->numActions; i++)
		nodeType[aNode(i)] = tAND;

	for(int iA = 0; iA < htn->numActions; iA++) {
		int nA = aNode(iA);
		for(int iF = 0; iF < htn->numPrecs[iA]; iF++) {
			int f = htn->precLists[iA][iF];
			int nF = fNode(f);
			N[nF].push_back(nA);
			Ninv[nA].push_back(nF);
		}
		for(int iF = 0; iF < htn->numAdds[iA]; iF++) {
			int f = htn->addLists[iA][iF];
			int nF = fNode(f);
			N[nA].push_back(nF);
			Ninv[nF].push_back(nA);
		}
	}

	for(int i = htn->numActions; i < htn->numTasks; i++) // abstract tasks
		nodeType[aNode(i)] = tOR;
	for(int i = 0; i < htn->numMethods; i++) // method nodes
		nodeType[mNode(i)] = tAND;

	for(int iM = 0; iM < htn->numMethods; iM++) {
		int nM = mNode(iM);
		int tAbs = htn->decomposedTask[iM];
		int nT = aNode(tAbs);
		N[nM].push_back(nT);
		Ninv[nT].push_back(nM);
		for(int iST = 0; iST < htn->numSubTasks[iM]; iST++) {
			int st = htn->subTasks[iM][iST];
			int nST = aNode(st);
			N[nST].push_back(nM);
			Ninv[nM].push_back(nST);
		}
	}

	if(loopback) {
		for(int a = 0; a < htn->numActions; a++) {
			// create one or-node per action
			int orNode = htn->numStateBits + htn->numTasks + htn->numMethods + a;
			nodeType[orNode] = tOR;
			N[orNode].push_back(aNode(a)); // like a precondition of a
			Ninv[aNode(a)].push_back(orNode);
			//N[aNode(a)].push_back(orNode); // like a precondition of a
			//Ninv[orNode].push_back(aNode(a));

			for(int im = 0; im < htn->stToMethodNum[a]; im++) {
				int m = htn->stToMethod[a][im];
				int mN = mNode(m);
				N[mN].push_back(orNode);
				Ninv[orNode].push_back(mN);
				//N[orNode].push_back(mN);
				//Ninv[mN].push_back(orNode);
			}
		}
	}

	fullSet = new bool[numNodes];
	LMs = new set<int>[numNodes];
	this->heap = new IntPairHeap<int>(numNodes / 4);

	temp = new set<int>;
	temp2 = new set<int>;
	flm = new set<int>;
	mlm = new set<int>;
	tlm = new set<int>;
	tasksInTNI = new int[htn->numTasks];
	//prettyPrintGraph();
}
void LMsInAndOrGraphs::prettyPrintGraph() {
	cout << "digraph {" << endl;
	for(int i = 0; i < numNodes; i++) {
		if(i == 0) {
			cout << "    subgraph cluster_STATE {" << endl;
		} else if (i == htn->numStateBits) {
			cout << "    }" << endl;
			cout << "    subgraph cluster_ACTIONS {" << endl;
		} else if (i == (htn->numStateBits + htn->numActions)) {
			cout << "    }" << endl;
		}
		cout << "    n" << i << "[";
		if(nodeType[i] == tAND) {
			cout << "shape=box";
		} else if(nodeType[i] == tOR) {
			cout << "shape=circle";
		}
		cout << ", ";

		if(isFNode(i)) {
			cout << "label=\"" << htn->factStrs[nodeToF(i)] <<"\"";
		} else if(isANode(i)) {
			cout << "label=\"" << htn->taskNames[nodeToA(i)] <<"\"";
		} else if (isMNode(i)) {
			cout << "label=\"" << htn->methodNames[nodeToM(i)] <<"\"";
		} else {
			cout << "label=\"\""; // empty
		}
		cout << "]" << endl;
	}
	cout << endl;
	for(int i = 0; i < numNodes; i++) {
		for(int j = 0; j < (int)N[i].size(); j++) {
			cout << "    n" << i << " -> " << "n" << N[i][j] << ";" << endl;
		}
	}

	cout << "}" << endl;
}

LMsInAndOrGraphs::~LMsInAndOrGraphs() {

	delete[] N;
	delete[] Ninv;
	delete[] nodeType;
	delete[] fullSet;
	delete[] LMs;
	delete temp;
	delete temp2;
	delete[] tasksInTNI;
}

void LMsInAndOrGraphs::generateAndOrLMs(searchNode* tnI){

	timeval tp;
	long startT = 0;
	if(!silent) {
		gettimeofday(&tp, NULL);
		startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	}
	for(int i = 0; i < numNodes; i++)
		fullSet[i] = true;

	for(int i = 0; i < htn->s0Size; i++){
		int n = fNode(htn->s0List[i]);
		LMs[n].clear();
		LMs[n].insert(n);
		fullSet[n] = false;
		nodeType[n] = tINIT;
		for (vector<int>::iterator it = N[n].begin(); it != N[n].end(); ++it) {
			heap->add(1, *it);
		}
	}
	for(int i = 0; i < htn->numPrecLessActions; i++){
		int task = htn->precLessActions[i];
		int n = aNode(task);
		LMs[n].clear();
		LMs[n].insert(n);
		fullSet[n] = false;
		for (vector<int>::iterator it = N[n].begin(); it != N[n].end(); ++it) {
			heap->add(1, *it);
		}
	}

	int intercount = 0;

	while(!heap->isEmpty()) {
		if(!silent) {
			if((intercount > 0) && ((intercount % 10000) == 0)) {
				cout << "Iterations: "<< intercount << endl;
			}
		}
		intercount++;
		int n = heap->topVal();

		heap->pop();
		bool changed = false;

		temp->clear();
		bool tFullSet;
		if (nodeType[n] == tAND) { // union of all parent sets
			tFullSet = false;
			for(vector<int>::iterator it = Ninv[n].begin(); it != Ninv[n].end(); ++it) {
				int n2 = *it;
				if(fullSet[n2]) {
					tFullSet = true;
					break;
				} else {
					for(set<int>::iterator it2 = LMs[n2].begin(); it2 != LMs[n2].end(); ++it2) {
						int lm = *it2;
						temp->insert(lm);
					}
				}
			}
		} else if (nodeType[n] == tOR) { // intersection of all parent sets
			bool first = true;
			tFullSet = true;
			for(vector<int>::iterator it = Ninv[n].begin(); it != Ninv[n].end(); ++it) {
				int n2 = *it;
				if (first) {
					first = false;
					if (fullSet[n2]){
						tFullSet = true;
					} else {
						for(set<int>::iterator it2 = LMs[n2].begin(); it2 != LMs[n2].end(); ++it2) {
							tFullSet = false;
							int lm = *it2;
							temp->insert(lm);
						}
					}
				} else if(!fullSet[n2]) {
					if(tFullSet){ // need to add all nodes
						tFullSet = false;
						for(set<int>::iterator it2 = LMs[n2].begin(); it2 != LMs[n2].end(); ++it2) {
							int lm = *it2;
							temp->insert(lm);
						}
					} else { // need to intersect
						temp2->clear();
						for(set<int>::iterator it2 = LMs[n2].begin(); it2 != LMs[n2].end(); ++it2) {
							int lm = *it2;
							if(temp->find(lm) != temp->end()) { // contained in both
								temp2->insert(lm);
							}
						}
						set<int>* temp3 = temp;
						temp = temp2;
						temp2 = temp3;
					}
				}
			}
		} else { // initial node
			// do nothing
			tFullSet = false;
			temp->clear(); // n will be added later
		}

		// update node LMs
		if(tFullSet){
			changed = (fullSet[n] == false);
			fullSet[n] = true;
		} else {
			temp->insert(n);
			if (fullSet[n] == true){
				changed = true;
			} else if(LMs[n].size() != temp->size()){
				changed = true;
			} else {
				for (set<int>::iterator it = temp->begin(); it != temp->end(); ++it) {
					if(LMs[n].find(*it) == LMs[n].end()) {
						changed = true;
						break;
					}
				}
			}
			if (changed) {
				fullSet[n] = false;
				LMs[n].clear();
				for (set<int>::iterator it = temp->begin(); it != temp->end(); ++it) {
					LMs[n].insert(*it);
				}
			}
		}
		if (changed) {
			for (vector<int>::iterator it = N[n].begin(); it != N[n].end(); ++it) {
				int key = numNodes;
				if(fullSet[n] == false)
					key = LMs[n].size();
				heap->add(key, *it);
			}
		}
	}

	if(!silent) {
		cout << "- number of nodes in AND/OR graph: " << numNodes << endl;
		cout << "- converged after processing " << intercount <<  " nodes" << endl;
	}

	// landmarks from state-based goal
	for (int i = 0; i < htn->gSize; i++) {
		int f = htn->gList[i];
		int n = fNode(f);
		for (set<int>::iterator it = LMs[n].begin(); it != LMs[n].end(); ++it) {
			int lm = *it;
			if (isFNode(lm)) {
				flm->insert(nodeToF(lm));
			} else if (isANode(lm)) {
				tlm->insert(nodeToA(lm));
			} else {
				assert(isMNode(nodeToM(lm)));
				mlm->insert(lm);
			}
		}
	}

	// landmarks from top level tasks
	for (int i = 0; i < htn->numTasks; i++)
		tasksInTNI[i] = 0;

	set<int> done;
	vector<planStep*> todoList;
	for (int i = 0; i < tnI->numPrimitive; i++)
		todoList.push_back(tnI->unconstraintPrimitive[i]);
	for (int i = 0; i < tnI->numAbstract; i++)
		todoList.push_back(tnI->unconstraintAbstract[i]);
	while (!todoList.empty()) {
		planStep* ps = todoList.back();
		todoList.pop_back();
		done.insert(ps->id);
		tasksInTNI[ps->task]++;
		for (int i = 0; i < ps->numSuccessors; i++) {
			planStep* succ = ps->successorList[i];
			bool included = done.find(succ->id) != done.end();
			if (!included)
				todoList.push_back(succ);
		}
	}

	for (int iT = 0; iT < htn->numTasks; iT++) {
		if (tasksInTNI[iT] == 0)
			continue;
		int n = aNode(iT);
		for (set<int>::iterator it = LMs[n].begin(); it != LMs[n].end(); ++it) {
			int lm = *it;
			if (isFNode(lm)) {
				flm->insert(nodeToF(lm));
			} else if (isANode(lm)) {
				tlm->insert(nodeToA(lm));
			} else {
				assert(isMNode(lm));
				mlm->insert(nodeToM(lm));
			}
		}
	}

	if(!silent) {
		gettimeofday(&tp, NULL);
		long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
		cout << "- time for LM extraction (ms) : " << (endT - startT) << endl;
	}
}

int LMsInAndOrGraphs::fNode(int i) {
	return i;
}

int LMsInAndOrGraphs::aNode(int i) {
	return htn->numStateBits + i;
}

int LMsInAndOrGraphs::mNode(int i) {
	return htn->numStateBits + htn->numTasks + i;
}

int LMsInAndOrGraphs::nodeToF(int i) {
 return i;
}

int LMsInAndOrGraphs::nodeToA(int i) {
	return i - htn->numStateBits;
}

int LMsInAndOrGraphs::nodeToM(int i) {
	return i - (htn->numStateBits + htn->numTasks);
}

bool LMsInAndOrGraphs::isFNode(int i) {
	return (i < htn->numStateBits);
}

bool LMsInAndOrGraphs::isANode(int i) {
	return (i >= htn->numStateBits) && (i < (htn->numStateBits + htn->numTasks));
}

bool LMsInAndOrGraphs::isMNode(int i) {
	return ((i >= (htn->numStateBits + htn->numTasks)) && (i < htn->numStateBits + htn->numTasks + htn->numMethods));
}

// Elkawkagy et al.'s recursive local landmarks

void LMsInAndOrGraphs::generateLocalLMs(Model* htn, searchNode* tnI){
	timeval tp;
	gettimeofday(&tp, NULL);
	long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	set<int> initialTasks;
	set<int> done;
	vector<planStep*> todoList;
	for (int i = 0; i < tnI->numPrimitive; i++)
		todoList.push_back(tnI->unconstraintPrimitive[i]);
	for (int i = 0; i < tnI->numAbstract; i++)
		todoList.push_back(tnI->unconstraintAbstract[i]);
	while (!todoList.empty()) {
		planStep* ps = todoList.back();
		todoList.pop_back();
		done.insert(ps->id);
		initialTasks.insert(ps->task);
		for (int i = 0; i < ps->numSuccessors; i++) {
			planStep* succ = ps->successorList[i];
			bool included = done.find(succ->id) != done.end();
			if (!included)
				todoList.push_back(succ);
		}
	}

	set<int>* lms = new set<int>();
	set<int>* collect = new set<int>();
	for (set<int>::iterator it = initialTasks.begin(); it != initialTasks.end(); ++it) {
		int task = *it;
		set<int>* newLMs = genLocalLMs(htn, task);
		for (set<int>::iterator it2 = newLMs->begin(); it2 != newLMs->end(); ++it2) {
			collect->insert(*it2);
			lms->insert(*it2);
		}
		delete newLMs;
	}
	while(!collect->empty()) {
		set<int>* lastRound = collect;
		collect = new set<int>();
		for (set<int>::iterator it = lastRound->begin(); it != lastRound->end(); ++it) {
			set<int>* newLMs = genLocalLMs(htn, *it);
			for (set<int>::iterator it2 = newLMs->begin(); it2 != newLMs->end(); ++it2) {
				int lm = *it2;
				if(lms->find(lm) == lms->end()){
					collect->insert(lm);
					lms->insert(lm);
				}
			}
		}
		delete lastRound;
	}

	tlm->clear();
	flm->clear();
	mlm->clear();

	for (int lm : *lms) {
		tlm->insert(lm);
	}

	gettimeofday(&tp, NULL);
	long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	cout << "- time for LM extraction (ms) : " << (endT - startT) << endl;
}

set<int>* LMsInAndOrGraphs::genLocalLMs(Model* htn, int task){
	set<int>* res = new set<int>();
	set<int>* res2 = new set<int>();
	bool first = true;
	for(int i = 0; i < htn->numMethodsForTask[task]; i++) {
		int m = htn->taskToMethods[task][i];
		if(first){
			for(int iST =0; iST < htn->numSubTasks[m]; iST++){
				int st = htn->subTasks[m][iST];
				res->insert(st);
			}
			first = false;
		} else {
			res2->clear();
			for(int iST =0; iST < htn->numSubTasks[m]; iST++){
				int st = htn->subTasks[m][iST];
				if(res->find(st) != res->end())
					res2->insert(st);
			}
			set<int>* temp = res;
			res = res2;
			res2 = temp;
		}
	}
	return res;
}

int LMsInAndOrGraphs::getNumLMs(){
	return (this->flm->size() + this->tlm->size() + this->mlm->size());
}

landmark** LMsInAndOrGraphs::getLMs(){
	landmark** lms = new landmark*[this->getNumLMs()];
	int i = 0;
	for(int f : *flm) {
		landmark* l = new landmark(atom, fact, 1);
		l->lm[0] = f;
		lms[i++] = l;
	}
	for(int t : *tlm) {
		landmark* l = new landmark(atom, task, 1);
		l->lm[0] = t;
		lms[i++] = l;
	}
	for(int m : *mlm) {
		landmark* l = new landmark(atom, METHOD, 1);
		l->lm[0] = m;
		lms[i++] = l;
	}
	assert(i == this->getNumLMs());
	return lms;
}


} /* namespace progression */
