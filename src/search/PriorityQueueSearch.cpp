/*
 * PriorityQueueSearch.cpp
 *
 *  Created on: 10.09.2017
 *      Author: Daniel Höller
 */

#include <iostream>
#include <stdlib.h>
#include <cassert>
#include <sys/time.h>

#include "PriorityQueueSearch.h"
#include "../ProgressionNetwork.h"
#include "../Model.h"
#include "../intDataStructures/FlexIntStack.h"

#if SEARCHTYPE == DFSEARCH
#include "StackFringe.h"
#elif SEARCHTYPE == BFSEARCH
#include "QueueFringe.h"
#endif

#ifdef PREFMOD
// preferred modifications
#include "AlternatingFringe.h"
#endif

#include <queue>
#include <map>
#include <algorithm>

namespace progression {


map<vector<uint64_t>, set<vector<int>>> visited;


vector<uint64_t> state2Int(vector<bool> & state){
	int pos = 0;
	uint64_t cur = 0;
	vector<uint64_t> vec;
	for (bool b : state){
		if (pos == 64){
			vec.push_back(cur);
			pos = 0;
			cur = 0;
		}

		if (b)
			cur |= uint64_t(1) << pos;

		pos++;
	}

	if (pos) vec.push_back(cur);

	return vec;
}


void dfsdfs(planStep * s, set<planStep*> & psp, map<planStep*,int> & prec){
	if (psp.count(s)) return;
	psp.insert(s);
	for (int ns = 0; ns < s->numSuccessors; ns++)
		prec[s->successorList[ns]]++, dfsdfs(s->successorList[ns],psp, prec);
}

int A = 0 ,B = 0;

bool insertVisi(searchNode * n){
	set<planStep*> psp; map<planStep*,int> prec;
	for (int a = 0; a < n->numAbstract; a++) dfsdfs(n->unconstraintAbstract[a], psp, prec);
	for (int a = 0; a < n->numPrimitive; a++) dfsdfs(n->unconstraintPrimitive[a], psp, prec);
	vector<int> seq;

	while (psp.size()){
		for (planStep * ps : psp)
			if (prec[ps] == 0){
				seq.push_back(ps->task);
				psp.erase(ps);
				for (int ns = 0; ns < ps->numSuccessors; ns++)
					prec[ps->successorList[ns]]--;
				break;
			}
	}

	//for (int x : seq) cout << x << " ";
	//cout << endl;

	//return true;

	A++;
	vector<uint64_t> ss = state2Int(n->state);
	if (visited[ss].count(seq)) return false;

	visited[ss].insert(seq);
	B++;


	//cout << A << " " << B << endl;
	return true;
}



PriorityQueueSearch::PriorityQueueSearch() {
	// TODO Auto-generated constructor stub

}

PriorityQueueSearch::~PriorityQueueSearch() {
	// TODO Auto-generated destructor stub
}

#ifdef TRACESOLUTION
pair<string,int> extractSolutionFromSearchNode(Model * htn, searchNode* tnSol){
	int sLength = 0;
	string sol = "";
	solutionStep* sost = tnSol->solution;
	bool done = sost == nullptr || sost->prev == nullptr;

	map<int,vector<pair<int,int>>> children;
	vector<pair<int,string>> decompositionStructure;

	int root = -1;

	while (!done) {
		sLength++;
		if (sost->method >= 0){
			pair<int,string> application;
			application.first = sost->mySolutionStepInstanceNumber;
			application.second = htn->taskNames[sost->task] + " -> " + htn->methodNames[sost->method];
			decompositionStructure.push_back(application);
			if (sost->task == htn->initialTask) root = application.first;
		} else {
			sol = to_string(sost->mySolutionStepInstanceNumber) + " " +
					htn->taskNames[sost->task] + "\n" + sol;
		}
		
		if (sost->mySolutionStepInstanceNumber != 0)
			children[sost->parentSolutionStepInstanceNumber].push_back(
					make_pair(
						sost->myPositionInParent,
						sost->mySolutionStepInstanceNumber));
		
		done = sost->prev == nullptr;
		sost = sost->prev;
	}

	sol = "==>\n" + sol;
	sol = sol + "root " + to_string(root) + "\n";
	for (auto x : decompositionStructure){
		sol += to_string(x.first) + " " + x.second;
		sort(children[x.first].begin(), children[x.first].end());
		for (auto [_,y] : children[x.first])
			sol += " " + to_string(y);
		sol += "\n";
	}

	sol += "<==";

	return make_pair(sol,sLength);
}
#endif


pair<string,int> printTraceOfSearchNode(Model* htn, searchNode* tnSol){
	int sLength = 0;
	string sol = "";
	solutionStep* sost = tnSol->solution;
	bool done = sost == nullptr || sost->prev == nullptr;
	while (!done) {
		sLength++;
		if (sost->method >= 0)
			sol = htn->methodNames[sost->method] + " @ "
					+ htn->taskNames[sost->task] + "\n" + sol;
		else
			sol = htn->taskNames[sost->task] + "\n" + sol;
		done = sost->prev == nullptr;
		sost = sost->prev;
	}

	return make_pair(sol,sLength);
}


void PriorityQueueSearch::search(Model* htn, searchNode* tnI, int timeLimit) {
	timeval tp;
	gettimeofday(&tp, NULL);
	long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	long currentT;
	long lastOutput = startT;
	bool reachedTimeLimit = false;
	const int checkAfter = CHECKAFTER;
	int lastCheck = 0;

	searchNode* tnSol = nullptr;
	bool continueSearch = true;

	cout << "Search Configuration" << endl;

#if SEARCHALG == PROGRESSIONORG
	cout << "- Using original progression algorithm" << endl;
#elif SEARCHALG == ICAPS18
	cout << "- Using ICAPS 2018 progression algorithm" << endl;
#elif SEARCHALG == JAIR19
	cout << "- Using JAIR 2019 progression algorithm" << endl;
#endif

	if (optimzeSol) {
		cout << "- After first solution is found, search is continued until time limit to find better solution." << endl;
	} else {
		cout << "- Search is stopped after first solution is found." << endl;
	}

#ifdef ASTAR
		if (GASTARWEIGHT != 1)
			cout << "- Greedy A* Search with weight " << GASTARWEIGHT << endl;
		else
			cout << "- A* Search" << endl;
#ifdef ASTARAC
		cout << "- Distance G is \"action costs\" instead of \"modification depth\"" << endl;
#else
		cout << "- Distance G is \"modification depth\"" << endl;
#endif
#else
		cout << "- Greedy Search" << endl;
#endif



#if SEARCHTYPE == DFSEARCH
	StackFringe fringe;
	assert(fringe.empty());
	if (insertVisi(tnI))
		fringe.push(tnI);
	assert(!fringe.empty());
#elif SEARCHTYPE == BFSEARCH
	QueueFringe fringe;
	if (insertVisi(tnI))
		fringe.push(tnI);
#else
#ifdef PREFMOD
	cout << "- using preferred modifications and an alternating fringe."
	<< endl;
	AlternatingFringe fringe;
	if (insertVisi(tnI))
		fringe.push(tnI, true);
#else // SEARCHTYPE == HEURISTICSEARCH
	cout << "- using priority queue as fringe." << endl;
	priority_queue<searchNode*, vector<searchNode*>, CmpNodePtrs> fringe;
	if (insertVisi(tnI))
		fringe.push(tnI);
#endif
#endif
	int numSearchNodes = 1;

	while (!fringe.empty()) {
		searchNode *n = fringe.top();
		assert(n != nullptr);
		fringe.pop();
#ifndef EARLYGOALTEST
		if (htn->isGoal(n)) {
			// A non-early goal test makes only sense in an optimal planning setting.
			// -> continuing search makes not really sense here
			gettimeofday(&tp, NULL);
			currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
			tnSol =	handleNewSolution(n, tnSol, currentT - startT);
			continueSearch = this->optimzeSol;
			if(!continueSearch)
				break;
		}
#endif


#if SEARCHALG == JAIR19
		if (n->numAbstract == 0) {
#endif
			for (int i = 0; i < n->numPrimitive; i++) {
				if (!htn->isApplicable(n, n->unconstraintPrimitive[i]->task))
					continue;
				searchNode *n2 = htn->apply(n, i);
				numSearchNodes++;
				if (!n2->goalReachable) { // progression has detected unsol
					delete n2;
					continue;
				}

				hF->setHeuristicValue(n2, n, n->unconstraintPrimitive[i]->task);

				assert(n2->goalReachable || (!htn->isGoal(n2))); // otherwise the heuristic is not save

				if (n2->goalReachable) {
#ifdef ASTAR
#ifdef ASTARAC
					n2->heuristicValue += (n2->actionCosts / GASTARWEIGHT);
#else
					n2->heuristicValue +=
							(n2->modificationDepth / GASTARWEIGHT);
//							(n2->mixedModificationDepth / GASTARWEIGHT);
#endif
#endif
#ifdef EARLYGOALTEST
					if (htn->isGoal(n2)) {
						gettimeofday(&tp, NULL);
						currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
						tnSol =	handleNewSolution(n2, tnSol, currentT - startT);
						continueSearch = this->optimzeSol;
						if(!continueSearch)
							break;
					} else
#endif
					if (insertVisi(n2))
						fringe.push(n2);

				} else {
					delete n2;
				}
			}
#if SEARCHALG == JAIR19
		}
#endif
		if (!continueSearch)
			break;
#if (SEARCHALG == JAIR19) || (SEARCHALG == ICAPS18)
		if (n->numAbstract > 0) {
			int decomposedStep = rand() % n->numAbstract;
			int task = n->unconstraintAbstract[decomposedStep]->task;
#elif (SEARCHALG == PROGRESSIONORG)
			for (int decomposedStep = 0; decomposedStep < n->numAbstract; decomposedStep++) {
				int task = n->unconstraintAbstract[decomposedStep]->task;
#endif
			for (int i = 0; i < htn->numMethodsForTask[task]; i++) {
				int method = htn->taskToMethods[task][i];
				searchNode *n2 = htn->decompose(n, decomposedStep, method);
				numSearchNodes++;
				if (!n2->goalReachable) { // decomposition has detected unsol
					delete n2;
					continue; // with next method
				}
				hF->setHeuristicValue(n2, n, decomposedStep, method);

				assert(n2->goalReachable || (!htn->isGoal(n2))); // otherwise the heuristic is not save

				if (n2->goalReachable) {
#ifdef ASTAR
#ifdef ASTARAC
					n2->heuristicValue += (n2->actionCosts / GASTARWEIGHT);
#else
					n2->heuristicValue +=
							(n2->modificationDepth / GASTARWEIGHT);
//							(n2->mixedModificationDepth / GASTARWEIGHT);
#endif
#endif
#ifdef EARLYGOALTEST
					if (htn->isGoal(n2)) {
						gettimeofday(&tp, NULL);
						currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
						tnSol =	handleNewSolution(n2, tnSol, currentT - startT);
						continueSearch = this->optimzeSol;
						if(!continueSearch)
							break;

					} else
#endif
						if (insertVisi(n2))
							fringe.push(n2);

				} else {
					delete n2;
				}
			}
		}
		int allnodes = numSearchNodes + htn->numOneModActions + htn->numOneModMethods + htn->numEffLessProg;

		if (allnodes - lastCheck >= checkAfter) {
			lastCheck = allnodes;

			gettimeofday(&tp, NULL);
			currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

			if (((currentT - lastOutput) / 1000) > 0) {
				cout << int((currentT - startT) / 1000) << "s generated nodes: "
						<< allnodes << " nodes/sec.: "
						<< int(
								double(allnodes) / (currentT - startT)
										* 1000) << " current heuristic: "
						<< n->heuristicValue << " mod.depth "
						<< n->modificationDepth << endl;
				lastOutput = currentT;
			}
			if ((timeLimit > 0) && ((currentT - startT) / 1000 > timeLimit)) {
				reachedTimeLimit = true;
				cout << "Reached time limit - stopping search." << endl;
				break;
			}
		}

		delete n;
	}
	gettimeofday(&tp, NULL);
	currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	cout << "Search Results" << endl;
	cout << "- Search time " << double(currentT - startT) / 1000 << " seconds"
			<< endl;
	cout << "- Generated "
			<< (numSearchNodes + htn->numOneModActions + htn->numOneModMethods
					+ htn->numEffLessProg) << " search nodes" << endl;
	cout << "  Calculated heuristic for " << numSearchNodes << " nodes" << endl;
	cout << "  One modifications " << (htn->numOneModActions + htn->numOneModMethods) << endl;
	cout << "  Effectless actions " << htn->numEffLessProg << endl;
	cout << "- including " << (htn->numOneModActions)
			<< " one modification actions" << endl;
	cout << "- including " << (htn->numOneModMethods)
			<< " one modification methods" << endl;
	cout << "- and       " << (htn->numEffLessProg)
			<< " progressions of effectless actions" << endl;
	cout << "- Generated "
			<< int(double(numSearchNodes) / (currentT - startT) * 1000)
			<< " nodes per second" << endl;
	cout << "- Final fringe contains " << fringe.size() << " nodes" << endl;
	if (this->foundSols > 1) {
		cout << "- Found " << this->foundSols << " solutions." << endl;
		cout << "  - first solution after " << this->firstSolTime << "ms." << endl;
		cout << "  - best solution after " << this->bestSolTime << "ms." << endl;
	}
	if (tnSol != nullptr) {
#ifdef TRACESOLUTION
		auto [sol,sLength] = extractSolutionFromSearchNode(htn,tnSol);
#else
		auto [sol,sLength] = printTraceOfSearchNode(htn,tnSol);
#endif
		cout << "- Status: Solved" << endl;
		cout << "- Found solution of length " << sLength << endl;
		cout << "- Total costs of actions: " << tnSol->actionCosts << endl
				<< endl;
		cout << sol << endl;
#ifdef TRACKLMSFULL
		assert(tnSol->lookForT->size == 0);
		assert(tnSol->lookForM->size == 0);
		assert(tnSol->lookForF->size == 0);
#endif
	} else if (reachedTimeLimit) {
		cout << "- Status: Timeout" << endl;
	} else {
		cout << "- Status: Proven unsolvable" << endl;
	}

#ifndef NDEBUG
	cout << "Deleting elements in fringe..." << endl;
	while (!fringe.empty()) {
		searchNode *n = fringe.top();
		fringe.pop();
		delete n;
	}
	delete tnSol;
	delete hF;
#endif
}

	searchNode* PriorityQueueSearch::handleNewSolution(searchNode* newSol, searchNode* oldSol, long time) {
		searchNode* res;
		foundSols++;
		if(oldSol == nullptr) {
			res = newSol;
			firstSolTime = time;
			bestSolTime = time;
			if (this->optimzeSol) {
				cout << "SOLUTION: (" << time << "ms) Found first solution with action costs of " << newSol->actionCosts << "." << endl;
			}
		} else if(newSol->actionCosts < oldSol->actionCosts) {
			// shall optimize until time limit, this is a better one
			bestSolTime = time;
			res = newSol;
			solImproved++;
			cout << "SOLUTION: (" << time << "ms) Found new solution with action costs of " << newSol->actionCosts << "." << endl;
		} else {
			//cout << "Found new solution with action costs of " << newSol->actionCosts << "." << endl;
			res = oldSol;
		}
		return res;
	}
}
