/*
 * hhDOfree.cpp
 *
 *  Created on: 27.09.2018
 *      Author: Daniel Höller
 */

#include "hhDOfree.h"

// default definition of ILP heuristics are not configured. Without this define, other heuristics lead to compile errors
#define INTVAR  ILOFLOAT
#define BOOLVAR ILOFLOAT

#if HEURISTIC == DOFREEILP
#define INTVAR  ILOINT
#define BOOLVAR ILOBOOL
#endif

#if HEURISTIC == DOFREELP
#define INTVAR  ILOFLOAT
#define BOOLVAR ILOFLOAT
#endif

#define MYVAR ILOINT

//#define OUTPUTLPMODEL
#define NAMEMODEL // assign human-readable names to each ILP variable

#ifdef DOFREE
namespace progression {

long pruned = 0;
long calculations = 0;
long incMCalcs = 0;
long incACalcs = 0;
//long maxval = 0;

#ifdef DOFINREMENTAL
hhDOfree::hhDOfree(Model* htn, searchNode *n) :
		model(this->lenv), v(this->lenv), cplex(this->model) {
#else
	hhDOfree::hhDOfree(Model* htn, searchNode *n) {
#endif

	preProReachable.init(htn->numTasks);
	this->pg = new planningGraph(htn);
	this->htn = htn;

	//cout << "- preparing tnI for DOF heuristic ... ";
	//countTNI(n, htn); // set the number of tasks in the initial task network
	//cout << "done." << endl;

	// init
	cout << "- initializing data structures for ILP model ... ";
	this->iUF = new int[htn->numStateBits];
	this->iUA = new int[htn->numTasks];
	this->iM = new int[htn->numMethods];

	int curI = 0;
	EStartI = new int[htn->numActions];

	// storing for a certain state feature the action that has it as add effect
	vector<int> EInvAction[htn->numStateBits];
	vector<int> EInvEffIndex[htn->numStateBits];

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
	iEInvActionIndex = new int*[htn->numStateBits];
	iEInvEffIndex = new int*[htn->numStateBits];

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

	this->iTP = new int[htn->numStateBits]; // time proposition
	this->iTA = new int[htn->numActions]; // time action
	cout << "done." << endl;

#ifdef INITSCCS
	cout
			<< "- initializing data structures to prevent disconnected components... ";
	sccNumIncommingMethods = new int*[htn->numSCCs];
	sccIncommingMethods = new int**[htn->numSCCs];

	// compute incomming methods for each scc
	for (int i = 0; i < htn->numCyclicSccs; i++) {
		int sccTo = htn->sccsCyclic[i];

		sccNumIncommingMethods[sccTo] = new int[htn->sccSize[sccTo]];
		for (int j = 0; j < htn->sccSize[sccTo]; j++) {
			sccNumIncommingMethods[sccTo][j] = 0;
		}
		sccIncommingMethods[sccTo] = new int*[htn->sccSize[sccTo]];

		set<int> incomingMethods[htn->sccSize[sccTo]];
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
			int task = htn->sccToTasks[sccTo][k];
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
		assert(sumSizes > 0); // some task out of the scc should be reached
	}

	sccNumInnerFromTo = new int*[htn->numSCCs];
	sccNumInnerToFrom = new int*[htn->numSCCs];
	sccNumInnerFromToMethods = new int**[htn->numSCCs];

	sccInnerFromTo = new int**[htn->numSCCs];
	sccInnerToFrom = new int**[htn->numSCCs];
	sccInnerFromToMethods = new int***[htn->numSCCs];

	for (int i = 0; i < htn->numCyclicSccs; i++) {
		int scc = htn->sccsCyclic[i];

		sccNumInnerFromTo[scc] = new int[htn->sccSize[scc]];
		sccNumInnerToFrom[scc] = new int[htn->sccSize[scc]];
		sccNumInnerFromToMethods[scc] = new int*[htn->sccSize[scc]];

		sccInnerFromTo[scc] = new int*[htn->sccSize[scc]];
		sccInnerToFrom[scc] = new int*[htn->sccSize[scc]];
		sccInnerFromToMethods[scc] = new int**[htn->sccSize[scc]];

		set<int> reachability[htn->sccSize[scc]][htn->sccSize[scc]];
		set<int> tasksToFrom[htn->sccSize[scc]];
		set<int> tasksFromTo[htn->sccSize[scc]];

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

			sccInnerFromToMethods[scc][iF] = new int*[htn->sccSize[scc]];
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
	}

	RlayerCurrent = new int[htn->sccMaxSize];
	RlayerPrev = new int[htn->sccMaxSize];
	Ivars = new int*[htn->sccMaxSize];
	for (int i = 0; i < htn->sccMaxSize; i++) {
		Ivars[i] = new int[htn->sccMaxSize];
	}
	cout << "done." << endl;
#endif

#ifdef DOFADDLMCUTLMS

#endif

#ifdef DOFLMS
	this->findLMs(n);
	for(int i =0; i < n->numfLMs;i++) {
		cout << "fact lm " << htn->factStrs[n->fLMs[i]] << endl;
	}
	for(int i =0; i < n->numtLMs;i++) {
		cout << "task lm " << htn->taskNames[n->tLMs[i]] << endl;
	}
	for(int i =0; i < n->nummLMs;i++) {
		cout  << "method lm " << htn->methodNames[n->mLMs[i]] << endl;
	}
#endif

#ifdef DOFINREMENTAL

	cplex.setParam(IloCplex::Param::Threads, 1);
	cplex.setParam(IloCplex::Param::TimeLimit, TIMELIMIT);
	cplex.setOut(lenv.getNullStream());
	cplex.setWarning(lenv.getNullStream());

	iS0 = new int[htn->numStateBits];
	ILPcurrentS0 = new int[htn->numStateBits];
	initialState = new IloExtractable[htn->numStateBits];
	setStateBits = new IloExtractable[htn->numStateBits];

	iTNI = new int[htn->numTasks];
	ILPcurrentTNI = new int[htn->numTasks];
	initialTasks = new IloExtractable[htn->numTasks];

	ILPcurrentFactReachability = new bool[htn->numStateBits];
	factReachability = new IloExtractable[htn->numStateBits];

	ILPcurrentTaskReachability = new bool[htn->numTasks];
	taskReachability = new IloExtractable[htn->numTasks];

	this->recreateModel(n); // create ILP model the first time
#endif
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

#if (DOFMODE == DOFRECREATE)

void hhDOfree::setHeuristicValue(searchNode *n, searchNode *parent,
		int action) {
	int h = this->recreateModel(n);
	n->goalReachable = (h != UNREACHABLE);
	n->heuristicValue = h;
}

#elif ((DOFMODE == DOFUPDATE) || (DOFMODE == DOFUPDATEWITHREACHABILITY))

void hhDOfree::setHeuristicValue(searchNode *n, searchNode *parent,
		int action) {
	updateS0(n);
	updateTNI(n);

#if (DOFMODE == DOFUPDATEWITHREACHABILITY)
	updateRechability(n);
#endif
	if (cplex.solve()) {
		n->heuristicValue = cplex.getObjValue();
	} else {
		n->heuristicValue = UNREACHABLE;
	}
	n->goalReachable = (n->heuristicValue != UNREACHABLE);
}

void hhDOfree::updateS0(searchNode *n) {
	for (int f = 0; f < htn->numStateBits; f++) {
		if (n->state[f] != ILPcurrentS0[f]) {
			ILPcurrentS0[f] = n->state[f];
			model.remove(this->initialState[f]);

			this->initialState[f] = (v[iS0[f]] == n->state[f]); // todo: do I need to delete something?
			model.add(this->initialState[f]);

			if (n->state[f]) { // was unset before, is now set -> need to add a constraint
				model.add(this->setStateBits[f]);
			} else { // is now false, was set before -> there should be no constraint
				model.remove(this->setStateBits[f]);
			}
		}
	}
}

void hhDOfree::updateTNI(searchNode *n) {
	// set the tasks in the initial task network
	for (int i = 0; i < htn->numTasks; i++) {
		if (this->ILPcurrentTNI[i] != n->TNIcount[i]) {
			model.remove(this->initialTasks[i]);
			ILPcurrentTNI[i] = n->TNIcount[i];
			this->initialTasks[i] = (v[iTNI[i]] == ILPcurrentTNI[i]);
			model.add(this->initialTasks[i]);
		}
	}
}

void hhDOfree::updateRechability(searchNode *n) {
	// calculate hierarchical planning graph
	this->updatePG(n);

	// add fact reachability to ILP
	for (int i = 0; i < htn->numStateBits; i++) {
		if (pg->factReachable(i) && (ILPcurrentFactReachability[i] == false)) {
			model.remove(this->factReachability[i]);
			ILPcurrentFactReachability[i] = true;
		} else if (!pg->factReachable(i) && ILPcurrentFactReachability[i] == true) {
			model.add(this->factReachability[i]);
			ILPcurrentFactReachability[i] = false;
		}
	}
	// add task reachability to ILP
	for (int i = 0; i < htn->numTasks; i++) {
		if (pg->taskReachable(i) && (ILPcurrentTaskReachability[i] == false)) {
			model.remove(this->taskReachability[i]);
			ILPcurrentTaskReachability[i] = true;
		} else if (!pg->taskReachable(i)
				&& (ILPcurrentTaskReachability[i] == true)) {
			model.add(this->taskReachability[i]);
			ILPcurrentTaskReachability[i] = false;
		}
	}
}
#endif

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

void hhDOfree::setHeuristicValue(searchNode *n, searchNode *parent, int absTask,
		int method) {
	this->setHeuristicValue(n, parent, -1);
}

/*
void hhDOfree::countTNI(searchNode* n, Model* htn) {
	// set the number of tasks in the initial task network
	n->TNIcount = new int[htn->numTasks];
	for (int i = 0; i < htn->numTasks; i++)
		n->TNIcount[i] = 0;
	set<int> done;
	vector<planStep*> todoList;
	for (int i = 0; i < n->numPrimitive; i++)
		todoList.push_back(n->unconstraintPrimitive[i]);
	for (int i = 0; i < n->numAbstract; i++)
		todoList.push_back(n->unconstraintAbstract[i]);
	while (!todoList.empty()) {
		planStep* ps = todoList.back();
		todoList.pop_back();
		done.insert(ps->id);
		n->TNIcount[ps->task]++;
		for (int i = 0; i < ps->numSuccessors; i++) {
			planStep* succ = ps->successorList[i];
			const bool included = done.find(succ->id) != done.end();
			if (!included)
				todoList.push_back(succ);
		}
	}
}*/

int hhDOfree::recreateModel(searchNode *n) {

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

#if (DOFMODE == DOFRECREATE)
	IloEnv lenv;
	IloNumVarArray v(lenv); //  all variables
	IloModel model(lenv);
#endif

	int iv = 0;
#ifndef DOFINREMENTAL
	// useful facts contains only a subset of all reachable
	for (int i = pg->usefulFactSet.getFirst(); i >= 0;
			i = pg->usefulFactSet.getNext()) {
#else
	for (int i = 0; i < this->htn->numStateBits; i++) {
		if (!pg->factReachable(i))
			continue;
		v.add(IloNumVar(lenv, 0, 1, BOOLVAR));
		iS0[i] = iv;
		v[iv].setName(("S0" + to_string(i)).c_str());
		iv++;
		// add constraints
		this->ILPcurrentS0[i] = n->state[i];
		this->initialState[i] = (v[iS0[i]] == n->state[i]);
		model.add(this->initialState[i]);
#endif
		v.add(IloNumVar(lenv, 0, 1, BOOLVAR));
		iUF[i] = iv;
#ifdef NAMEMODEL
		v[iv].setName(("UF" + to_string(i)).c_str());
#endif

#if (DOFMODE == DOFUPDATEWITHREACHABILITY)
		factReachability[i] = (v[iUF[i]] == 0); // only added when unreachable -> therefore set of 0
		ILPcurrentFactReachability[i] = true;
#endif
		iv++;
#ifdef DOFINREMENTAL
		this->setStateBits[i] = (v[iUF[i]] == 1);
		if (n->state[i]) { // only added when set -> therefore set to 1
			model.add(this->setStateBits[i]);
		}
#endif
	}

	for (int i = pg->reachableTasksSet.getFirst(); i >= 0;
			i = pg->reachableTasksSet.getNext()) {
#ifdef DOFINREMENTAL
		v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
		iTNI[i] = iv;
		v[iv].setName(("TNI" + to_string(i)).c_str());
		iv++;
#endif
		v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
		iUA[i] = iv;
#ifdef NAMEMODEL
		v[iv].setName(("UA" + to_string(i)).c_str());
#endif
#ifdef DOFINREMENTAL
		this->taskReachability[i] = (v[iUA[i]] == 0); // only added when unreachable
		this->ILPcurrentTaskReachability[i] = true;
#endif
		iv++;
	}

	for (int i = pg->reachableMethodsSet.getFirst(); i >= 0;
			i = pg->reachableMethodsSet.getNext()) {
		v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
		iM[i] = iv;
#ifdef NAMEMODEL
		v[iv].setName(("M" + to_string(i)).c_str());
#endif
		iv++;
	}

	for (int a = pg->reachableTasksSet.getFirst();
			(a >= 0) && (a < htn->numActions); a =
					pg->reachableTasksSet.getNext()) {
		for (int ai = 0; ai < htn->numAdds[a]; ai++) {
			if (!pg->usefulFactSet.get(htn->addLists[a][ai]))
				continue;
			v.add(IloNumVar(lenv, 0, 1, BOOLVAR));
			iE[EStartI[a] + ai] = iv;
#ifdef NAMEMODEL
			v[iv].setName(("xE" + to_string(a) + "x" + to_string(ai)).c_str());
#endif
			iv++;
		}
	}

#ifndef DOFTR
	for (int i = pg->usefulFactSet.getFirst(); i >= 0;
			i = pg->usefulFactSet.getNext()) {
		v.add(IloNumVar(lenv, 0, htn->numActions, INTVAR));
		iTP[i] = iv;
#ifdef NAMEMODEL
		v[iv].setName(("TP" + to_string(i)).c_str());
#endif
		iv++;
	}
	for (int a = pg->reachableTasksSet.getFirst();
			(a >= 0) && (a < htn->numActions); a =
					pg->reachableTasksSet.getNext()) {
		v.add(IloNumVar(lenv, 0, htn->numActions, INTVAR));
		iTA[a] = iv;
#ifdef NAMEMODEL
		v[iv].setName(("TA" + to_string(a)).c_str());
#endif
		iv++;
	}
#endif

	// create main optimization function
	IloNumExpr mainExp(lenv);

	for (int i = pg->reachableTasksSet.getFirst();
			(i >= 0) && (i < htn->numActions); i =
					pg->reachableTasksSet.getNext()) {
		int costs = htn->actionCosts[i];
		mainExp = mainExp + (costs * v[iUA[i]]);
	}

	for (int i = pg->reachableMethodsSet.getFirst(); i >= 0;
			i = pg->reachableMethodsSet.getNext()) {
		int costs = 1;
		mainExp = mainExp + (costs * v[iM[i]]);
	}

	model.add(IloMinimize(lenv, mainExp));

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

#ifdef DOFINREMENTAL
		c4 = c4 + v[iS0[f]];
#endif

		for (int i = 0; i < EInvSize[f]; i++) {
			int a = iEInvActionIndex[f][i];
			int iAdd = iEInvEffIndex[f][i];
			if (pg->taskReachable(a))
				c4 = c4 + v[iE[EStartI[a] + iAdd]];
		}
		model.add(c4 - v[iUF[f]] == 0);
	}

#ifndef DOFTR
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
#endif

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
#ifdef DOFINREMENTAL
		// add and set variable
		model.add(tup[i] + v[iTNI[i]] == v[iUA[i]]);
		ILPcurrentTNI[i] = n->TNIcount[i];
		initialTasks[i] = (v[iTNI[i]] == ILPcurrentTNI[i]);
		model.add(initialTasks[i]);
#else
		int iOfT = iu.indexOf(n->containedTasks, 0, n->numContainedTasks - 1, i);
		if (iOfT >= 0){
			model.add(tup[i] + n->containedTaskCount[iOfT] == v[iUA[i]]);
		} else {
			model.add(tup[i] + 0 == v[iUA[i]]);
		}
		//model.add(tup[i] + n->TNIcount[i] == v[iUA[i]]); // set directly
#endif
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

	// SCC stuff
#ifdef TREATSCCS

	for (int iSCC = 0; iSCC < htn->numCyclicSccs; iSCC++) {
		int scc = htn->sccsCyclic[iSCC];

		// initial layer R0
		// name schema: R-SCC-Time-Task
		for (int iT = 0; iT < htn->sccSize[scc]; iT++) {
			int task = htn->sccToTasks[scc][iT];
			if (!pg->taskReachable(task))
				continue;
			v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
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
			if(iOfT >= 0) {
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
			int* Rtemp = RlayerPrev;
			RlayerPrev = RlayerCurrent;
			RlayerCurrent = Rtemp;

			// create variables for ILP (needs to be done first, there may be forward references)
			for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) {
				int task = htn->sccToTasks[scc][iTask];
				if (!pg->taskReachable(task))
					continue;
				v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
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
					v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
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
#endif

#ifdef DOFLMS
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

	int res = -1;
#if (DOFMODE == DOFRECREATE)
	IloCplex cplex(model);
	//cplex.exportModel("/home/dh/Schreibtisch/temp/model-01.lp");

	cplex.setParam(IloCplex::Param::Threads, 1);
	cplex.setParam(IloCplex::Param::TimeLimit, TIMELIMIT / CHECKAFTER);
	//cplex.setParam(IloCplex::MIPEmphasis, CPX_MIPEMPHASIS_FEASIBILITY); // focus on feasability
	//cplex.setParam(IloCplex::IntSolLim, 1); // stop after first integer solution

	cplex.setOut(lenv.getNullStream());
	cplex.setWarning(lenv.getNullStream());

	if (cplex.solve()) {
		res = cplex.getObjValue();
		/*} else if (cplex.getStatus() == IloAlgorithm::Status::Unknown) {
		 cout << "value: time-limit" << endl;
		 res = 0;*/
	} else {
		res = UNREACHABLE;
	}

	lenv.end();
#endif
	return res;
}

void hhDOfree::printInfo(){
	cout << "Using Delete and Ordering Relaxed heuristic [heu=DOR]." << endl;
#if HEURISTIC == DOFREEILP
cout << "- using NP-hard calculation [heupar=ilp]" << endl;
#elif HEURISTIC == DOFREELP
cout << "- using relaxed P calculation [heupar=lp]" << endl;
#endif

#if (DOFMODE == DOFRECREATE)
cout << "- model is recreated in every search node (adding reachability information) [heupar=recreate]" << endl;
#elif (DOFMODE == DOFUPDATE)
cout << "- model is updated (without adding reachability information) [heupar=updatenori]" << endl;
#elif (DOFMODE == DOFUPDATEWITHREACHABILITY)
cout << "- model is updated (adding reachability information) [heupar=updatenori]" << endl;
#endif

#ifdef TREATSCCS
cout << "- preventing disconnected components is enabled [heupar=pdcOn]"
		<< endl;
#else
cout << "- preventing disconnected components is disabled [heupar=pdcOff]" << endl;
#endif

#ifdef DOFTR
cout << "- planning graph is time relaxed (omitting C5 and C6) [heupar=pgtrOn]" << endl;
#else
cout << "- planning graph is NOT time relaxed [heupar=pgtrOff]" << endl;
#endif

#ifdef DOFLMS
cout << "- adding landmarks to (I)LP model [heupar=lmOn]" << endl;
#else
cout << "- landmarks are NOT added to (I)LP model [heupar=lmOff]" << endl;
#endif
}

#ifdef DOFLMS
void hhDOfree::findLMs(searchNode *n) {
	cout << "- searching for landmarks in ILP (full model, NP calculation)" << endl;
	timeval tp;
	gettimeofday(&tp, NULL);
	long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

	set<int>* tLMcandidates = new set<int>;
	set<int>* tLMs = new set<int>;
	set<int>* tUNRcandidates = new set<int>;
	set<int>* tUNRs = new set<int>;

	set<int>* mLMcandidates = new set<int>;
	set<int>* mLMs = new set<int>;
	set<int>* mUNRcandidates = new set<int>;
	set<int>* mUNRs = new set<int>;

	set<int>* fLMcandidates = new set<int>;
	set<int>* fLMs = new set<int>;
	set<int>* fUNRcandidates = new set<int>;
	set<int>* fUNRs = new set<int>;

	// add candidates
	for(int i = 0; i < htn->numTasks; i++){
		tLMcandidates->insert(i);
		tUNRcandidates->insert(i);
	}
	for(int i = 0; i < htn->numMethods; i++){
		mLMcandidates->insert(i);
		mUNRcandidates->insert(i);
	}
	for(int i = 0; i < htn->numStateBits; i++){
		fLMcandidates->insert(i);
		fUNRcandidates->insert(i);
	}

	// find landmarks
	for(int i = 0; i < htn->numTasks;i++) {
		bool isElem = tLMcandidates->find(i) != tLMcandidates->end();
		if(!isElem)
			continue;
		ilpLMs(n,taskLM,i,0,
				tLMcandidates,tLMs,tUNRcandidates,tUNRs,
				mLMcandidates,mLMs,mUNRcandidates,mUNRs,
				fLMcandidates,fLMs,fUNRcandidates,fUNRs);
	}

	// find landmarks
	for(int i = 0; i < htn->numMethods;i++) {
		bool isElem = mLMcandidates->find(i) != mLMcandidates->end();
		if(!isElem)
			continue;
		ilpLMs(n,methLM,i,0,
				tLMcandidates,tLMs,tUNRcandidates,tUNRs,
				mLMcandidates,mLMs,mUNRcandidates,mUNRs,
				fLMcandidates,fLMs,fUNRcandidates,fUNRs);
	}

	// find landmarks
	for(int i = 0; i < htn->numStateBits;i++) {
		bool isElem = fLMcandidates->find(i) != fLMcandidates->end();
		if(!isElem)
			continue;
		ilpLMs(n,factLM,i,0,
				tLMcandidates,tLMs,tUNRcandidates,tUNRs,
				mLMcandidates,mLMs,mUNRcandidates,mUNRs,
				fLMcandidates,fLMs,fUNRcandidates,fUNRs);
	}

	/*
	// find unreachables
	for(int i = 0; i < htn->numTasks;i++) {
		bool isElem = tUNRcandidates->find(i) != tUNRcandidates->end();
		if(!isElem)
			continue;
		ilpLMs(n,taskLM,i,1,
				tLMcandidates,tLMs,tUNRcandidates,tUNRs,
				mLMcandidates,mLMs,mUNRcandidates,mUNRs,
				fLMcandidates,fLMs,fUNRcandidates,fUNRs);
	}

	for(int i = 0; i < htn->numMethods;i++) {
		bool isElem = mUNRcandidates->find(i) != mUNRcandidates->end();
		if(!isElem)
			continue;
		ilpLMs(n,methLM,i,1,
				tLMcandidates,tLMs,tUNRcandidates,tUNRs,
				mLMcandidates,mLMs,mUNRcandidates,mUNRs,
				fLMcandidates,fLMs,fUNRcandidates,fUNRs);
	}*/

	n->tLMs = new int[tLMs->size()];
	n->mLMs = new int[mLMs->size()];
	n->fLMs = new int[fLMs->size()];

	for(int tlm : *tLMs) {
		n->tLMs[n->numtLMs++] = tlm;
	}

	for(int mlm : *mLMs) {
		n->mLMs[n->nummLMs++] = mlm;
	}

	for(int flm : *fLMs) {
		n->fLMs[n->numfLMs++] = flm;
	}

	gettimeofday(&tp, NULL);
	long currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	cout << "  - found " << tLMs->size() << " task landmarks" << endl;
	cout << "  - found " << mLMs->size() << " method landmarks" << endl;
	cout << "  - found " << fLMs->size() << " fact landmarks" << endl;
	//cout << "- Found " << tUNRs->size() << " tasks that can not appear in a solution" << endl;
	//cout << "- Found " << mUNRs->size() << " methods that can not appear in a solution" << endl;
	//cout << "- Found " << fUNRs->size() << " facts that can not appear in a solution" << endl;
	cout << "- time for LM check: " << double(currentT - startT) / 1000 << " seconds" << endl;

	delete tLMcandidates;
	delete tLMs;
	delete tUNRcandidates;
	delete tUNRs;
	delete mLMcandidates;
	delete mLMs;
	delete mUNRcandidates;
	delete mUNRs;
	delete fLMcandidates;
	delete fLMs;
	delete fUNRcandidates;
	delete fUNRs;
}

void hhDOfree::ilpLMs(searchNode *n,int type, int id, int value, set<int>* tLMcandidates, set<int>* tLMs, set<int>* tUNRcandidates, set<int>* tUNRs,
		set<int>* mLMcandidates, set<int>* mLMs, set<int>* mUNRcandidates, set<int>* mUNRs,
		set<int>* fLMcandidates, set<int>* fLMs, set<int>* fUNRcandidates, set<int>* fUNRs) {

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

	// calculate reachability
	pg->calcReachability(n->state, preProReachable);

	if ((type == factLM) && (!pg->usefulFactSet.get(id))) {
		return;
	}

	IloEnv lenv;
	IloNumVarArray v(lenv); //  all variables
	int iv = 0;

	// create vars
	for (int i = pg->usefulFactSet.getFirst(); i >= 0;
			i = pg->usefulFactSet.getNext()) {
		v.add(IloNumVar(lenv, 0, 1, BOOLVAR));
		iUF[i] = iv;
		v[iv].setName(("UF" + to_string(i)).c_str());
		iv++;
	}

	for (int i = pg->reachableTasksSet.getFirst(); i >= 0;
			i = pg->reachableTasksSet.getNext()) {
		v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
		iUA[i] = iv;
		v[iv].setName(("UA" + to_string(i)).c_str());
		iv++;
	}

	for (int i = pg->reachableMethodsSet.getFirst(); i >= 0;
			i = pg->reachableMethodsSet.getNext()) {
		v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
		iM[i] = iv;
		v[iv].setName(("M" + to_string(i)).c_str());
		iv++;
	}

	for (int a = pg->reachableTasksSet.getFirst();
			(a >= 0) && (a < htn->numActions); a =
					pg->reachableTasksSet.getNext()) {
		for (int ai = 0; ai < htn->numAdds[a]; ai++) {
			if (!pg->usefulFactSet.get(htn->addLists[a][ai]))
				continue;
			v.add(IloNumVar(lenv, 0, 1, BOOLVAR));
			iE[EStartI[a] + ai] = iv;
			v[iv].setName(("xE" + to_string(a) + "x" + to_string(ai)).c_str());
			iv++;
		}
	}

	for (int i = pg->usefulFactSet.getFirst(); i >= 0;
			i = pg->usefulFactSet.getNext()) {
		v.add(IloNumVar(lenv, 0, htn->numActions, INTVAR));
		iTP[i] = iv;
		v[iv].setName(("TP" + to_string(i)).c_str());
		iv++;
	}
	for (int a = pg->reachableTasksSet.getFirst();
			(a >= 0) && (a < htn->numActions); a =
					pg->reachableTasksSet.getNext()) {
		v.add(IloNumVar(lenv, 0, htn->numActions, INTVAR));
		iTA[a] = iv;
		v[iv].setName(("TA" + to_string(a)).c_str());
		iv++;
	}

	// create main optimization function
	IloModel lmodel(lenv);
	IloNumExpr mainExp(lenv);

	for (int i = pg->reachableTasksSet.getFirst();
			(i >= 0) && (i < htn->numActions); i =
					pg->reachableTasksSet.getNext()) {
		int costs = htn->actionCosts[i];
		mainExp = mainExp + (costs * v[iUA[i]]);
	}

	for (int i = pg->reachableMethodsSet.getFirst(); i >= 0;
			i = pg->reachableMethodsSet.getNext()) {
		int costs = 1;
		mainExp = mainExp + (costs * v[iM[i]]);
	}

	lmodel.add(IloMinimize(lenv, mainExp));

	// C1
	for (int i = 0; i < htn->gSize; i++) {
		if (pg->usefulFactSet.get(htn->gList[i]))
			lmodel.add(v[iUF[htn->gList[i]]] == 1);
	}

	for (int a = pg->reachableTasksSet.getFirst();
			(a >= 0) && (a < htn->numActions); a =
					pg->reachableTasksSet.getNext()) {
		// C2
		for (int i = 0; i < htn->numPrecs[a]; i++) {
			int prec = htn->precLists[a][i]; // precs are always useful
			if (pg->usefulFactSet.get(prec))
				lmodel.add(100 * v[iUF[prec]] >= v[iUA[a]]); // TODO: how large must the const be?
		}
		// C3
		for (int iAdd = 0; iAdd < htn->numAdds[a]; iAdd++) {
			int fAdd = htn->addLists[a][iAdd];
			if (pg->usefulFactSet.get(fAdd))
				lmodel.add(v[iUA[a]] - v[iE[EStartI[a] + iAdd]] >= 0);
		}
	}

	// C4
	for (int f = pg->usefulFactSet.getFirst(); f >= 0;
			f = pg->usefulFactSet.getNext()) {
		//IloNumExpr c4(v[is0Vars[f]]);
		IloNumExpr c4(lenv);

		for (int i = 0; i < EInvSize[f]; i++) {
			int a = iEInvActionIndex[f][i];
			int iAdd = iEInvEffIndex[f][i];
			if (pg->taskReachable(a))
				c4 = c4 + v[iE[EStartI[a] + iAdd]];
		}
		lmodel.add(c4 - v[iUF[f]] == 0);
	}

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
				lmodel.add(v[iTA[a]] - v[iTP[prec]] >= 0);
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
			lmodel.add(
					v[iTA[a]] + 1
							<= v[iTP[add]]
									+ (htn->numActions + 1)
											* (1 - v[iE[EStartI[a] + iadd]]));
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
		lmodel.add(tup[i] + n->TNIcount[i] == v[iUA[i]]);

		if (i >= htn->numActions) {
			IloExpr x(lenv);
			for (int iMeth = 0; iMeth < htn->numMethodsForTask[i]; iMeth++) {
				int m = htn->taskToMethods[i][iMeth];
				if (pg->methodReachable(m)) {
					x = x + v[iM[m]];
				}
			}
			lmodel.add(x == v[iUA[i]]);
		}
	}

	// SCC stuff
	for (int iSCC = 0; iSCC < htn->numCyclicSccs; iSCC++) {
		int scc = htn->sccsCyclic[iSCC];

		// initial layer R0
		// name schema: R-SCC-Time-Task
		for (int iT = 0; iT < htn->sccSize[scc]; iT++) {
			int task = htn->sccToTasks[scc][iT];
			if(!pg->taskReachable(task))
				continue;
			v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
			v[iv].setName(("R_" + to_string(scc) + "_0_" + to_string(iT)).c_str());
			RlayerCurrent[iT] = iv++;
		}

		// C9
		for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) {
			int task = htn->sccToTasks[scc][iTask];
			if(!pg->taskReachable(task))
				continue;
			IloExpr c9(lenv);
			c9 = c9 + n->TNIcount[task];
			for (int iMeth = 0; iMeth < sccNumIncommingMethods[scc][iTask];
					iMeth++) {
				int m = sccIncommingMethods[scc][iTask][iMeth];
				if (pg->methodReachable(m))
					c9 = c9 + v[iM[m]];
			}
			lmodel.add(c9 >= v[RlayerCurrent[iTask]]);
		}

		for (int iTime = 1; iTime < htn->sccSize[scc]; iTime++) {
			int* Rtemp = RlayerPrev;
			RlayerPrev = RlayerCurrent;
			RlayerCurrent = Rtemp;

			// create variables for ILP (needs to be done first, there may be forward references)
			for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) {
				int task = htn->sccToTasks[scc][iTask];
				if(!pg->taskReachable(task))
					continue;
				v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
				v[iv].setName(
						("R_" + to_string(scc) + "_" + to_string(iTime) + "_"
								+ to_string(iTask)).c_str());
				RlayerCurrent[iTask] = iv++;
				for (int iTask2 = 0; iTask2 < htn->sccSize[scc]; iTask2++) {
					int task2 = htn->sccToTasks[scc][iTask2];
					if(!pg->taskReachable(task2))
						continue;
					v.add(IloNumVar(lenv, 0, INT_MAX, INTVAR));
					v[iv].setName(
							("I_" + to_string(scc) + "_" + to_string(iTime)
									+ "_" + to_string(iTask) + "_"
									+ to_string(iTask2)).c_str());
					Ivars[iTask][iTask2] = iv++;
				}
			}

			// create constraints C10, C11 and C12
			for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) { // t in the paper
				// C10
				int task = htn->sccToTasks[scc][iTask];
				if(!pg->taskReachable(task))
					continue;
				IloExpr c10(lenv);
				c10 = c10 + v[RlayerPrev[iTask]];

				for (int k = 0; k < sccNumInnerToFrom[scc][iTask]; k++) {
					int iTask2 = sccInnerToFrom[scc][iTask][k];
					int task2 = htn->sccToTasks[scc][iTask2];
					if(!pg->taskReachable(task2))
						continue;
					c10 = c10 + v[Ivars[iTask2][iTask]];
				}
				lmodel.add(v[RlayerCurrent[iTask]] <= c10);

				for (int iTask2 = 0; iTask2 < htn->sccSize[scc]; iTask2++) { // t' in the paper
					int task2 = htn->sccToTasks[scc][iTask2];
					if(!pg->taskReachable(task2))
						continue;

					// C11
					lmodel.add(v[Ivars[iTask2][iTask]] <= v[RlayerPrev[iTask2]]);

					// C12
					IloExpr c12(lenv);
					c12 = c12 + 0;
					for (int k = 0; k < sccNumInnerFromToMethods[scc][iTask2][iTask];k++) {
						int m = sccInnerFromToMethods[scc][iTask2][iTask][k];
						if(!pg->methodReachable(m))
							continue;
						c12 = c12 + v[iM[m]];
					}
					lmodel.add(v[Ivars[iTask2][iTask]] <= c12);
				}
			}
		}

		// C13
		for (int iTask = 0; iTask < htn->sccSize[scc]; iTask++) {
			int task = htn->sccToTasks[scc][iTask];
			if(!pg->taskReachable(task))
				continue;
			lmodel.add(v[iUA[task]] <= v[RlayerCurrent[iTask]] * 100); // todo: what is a sufficiently high number?
		}
	}

	for (int tLM : *tLMs) {
		lmodel.add(v[iUA[tLM]] >= 1);
	}
	for (int mLM : *mLMs) {
		lmodel.add(v[iM[mLM]] >= 1);
	}
	for (int fLM : *fLMs) {
		lmodel.add(v[iUF[fLM]] >= 1);
	}
	for (int tLM : *tUNRs) {
		lmodel.add(v[iUA[tLM]] == 0);
	}
	for (int mLM : *mUNRs) {
		lmodel.add(v[iM[mLM]] == 0);
	}

	if(type == taskLM){
		if (value == 0){
			lmodel.add(v[iUA[id]] == 0);
		} else {
			lmodel.add(v[iUA[id]] >= 1);
		}
	} else 	if(type == methLM){
		if (value == 0){
			lmodel.add(v[iM[id]] == 0);
		} else {
			lmodel.add(v[iM[id]] >= 1);
		}
	} else 	if(type == factLM){
		if (value == 0) {
			lmodel.add(v[iUF[id]] == 0);
		} else {
			lmodel.add(v[iUF[id]] >= 1);
		}
	}

	IloCplex cplex(lmodel);
	cplex.setParam(IloCplex::Param::Threads, 1);
	cplex.setParam(IloCplex::Param::TimeLimit, 1800);
	cplex.setOut(lenv.getNullStream());
	cplex.setWarning(lenv.getNullStream());

	if (cplex.solve()) {
		for(int i = 0; i < htn->numTasks; i++){
			int val = round(cplex.getValue(v[iUA[i]]));
			if(val > 0) {
				tUNRcandidates->erase(i);
			} else {
				tLMcandidates->erase(i);
			}
		}

		for(int i = 0; i < htn->numMethods; i++){
			int val = round(cplex.getValue(v[iM[i]]));
			if(val > 0){
				mUNRcandidates->erase(i);
			} else {
				mLMcandidates->erase(i);
			}
		}

		for(int i = 0; i < htn->numStateBits; i++){
			if (!pg->usefulFactSet.get(i))
				continue;
			int val = round(cplex.getValue(v[iUF[i]]));
			if(val > 0){
				fUNRcandidates->erase(i);
			} else {
				fLMcandidates->erase(i);
			}
		}
	} else {
		if(type == taskLM){
			if(value == 0) {
				tLMs->insert(id);
				tLMcandidates->erase(id);
				tUNRcandidates->erase(id);
			} else {
				tUNRs->insert(id);
				tLMcandidates->erase(id);
				tUNRcandidates->erase(id);
				for(int k = 0; k < htn->numMethodsForTask[id]; k++) {
					int m = htn->taskToMethods[id][k];
					mUNRs->insert(m);
					mUNRcandidates->erase(m);
					mLMcandidates->erase(m);
				}
				// methods that have this task as subtask
				for(int k = 0; k < pg->stToMethodNum[id]; k++) {
					int m = pg->stToMethod[id][k];
					mUNRs->insert(m);
					mUNRcandidates->erase(m);
					mLMcandidates->erase(m);
				}
			}
		} else if(type == methLM){
			if(value == 0) {
				mLMs->insert(id);
				mLMcandidates->erase(id);
				mUNRcandidates->erase(id);
			} else {
				mUNRs->insert(id);
				mLMcandidates->erase(id);
				mUNRcandidates->erase(id);
			}
		} else if(type == factLM){
			if(value == 0) {
				fLMs->insert(id);
				fLMcandidates->erase(id);
				fUNRcandidates->erase(id);
			} else {
				fUNRs->insert(id);
				fLMcandidates->erase(id);
				fUNRcandidates->erase(id);
			}
		}
	}
	lenv.end();
}
#endif

} /* namespace progression */
#endif
