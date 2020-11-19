/*
 * PriorityQueueSearch.cpp
 *
 *  Created on: 10.09.2017
 *      Author: Daniel Höller
 */

#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <cassert>
#include <chrono>
#include <fstream>
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
#include <bitset>
#include "primeNumbers.h"

namespace progression {

#ifndef SAVESEARCHSPACE 
map<vector<uint64_t>, set<vector<int>>> visited;
#else
map<vector<uint64_t>, map<vector<int>,int>> visited;
#endif

map<vector<uint64_t>, set<int>> visited2;


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


void dfsdfs(planStep * s, int depth, set<planStep*> & psp, set<pair<int,int>> & orderpairs, map<int,set<planStep*>> & layer){
	if (psp.count(s)) return;
	psp.insert(s);
	layer[depth].insert(s);
	for (int ns = 0; ns < s->numSuccessors; ns++){
		orderpairs.insert({s->task,s->successorList[ns]->task});
		dfsdfs(s->successorList[ns], depth + 1, psp, orderpairs, layer);
	}
}


void to_dfs(planStep * s, vector<int> & seq){
	//cout << s->numSuccessors << endl;
	assert(s->numSuccessors <= 1);
	seq.push_back(s->task);
	if (s->numSuccessors == 0) return;
	to_dfs(s->successorList[0],seq);
}


int A = 0, B = 0;
double time = 0;
bool useTotalOrderMode;


map<tuple< vector<uint64_t>, map<int,map<int,int>>, set<pair<int,int>> > , vector<searchNode*> > po_occ;


bool matchingDFS(searchNode* one, searchNode* other, planStep* oneStep, planStep* otherStep, map<planStep*, planStep*> mapping, map<planStep*, planStep*> backmapping);


bool matchingGenerate(
		searchNode* one, searchNode* other,
		map<planStep*, planStep*> & mapping, map<planStep*, planStep*> & backmapping,
		map<int,set<planStep*>> oneNextTasks,
		map<int,set<planStep*>> otherNextTasks,
		unordered_set<int> tasks
		){
	if (tasks.size() == 0)
		return true; // done, all children

	if (oneNextTasks[*tasks.begin()].size() == 0){
		tasks.erase(*tasks.begin());
		return matchingGenerate(one,other,mapping,backmapping,oneNextTasks,otherNextTasks,tasks);
	}

	int task = *tasks.begin();

	planStep* psOne = *oneNextTasks[task].begin();
	oneNextTasks[task].erase(psOne);

	// possible partners
	for (planStep * psOther : otherNextTasks[task]){
		if (mapping.count(psOne) && mapping[psOne] != psOther) continue;
		if (backmapping.count(psOther) && backmapping[psOther] != psOne) continue;

		bool subRes = true;
		map<planStep*, planStep*> subMapping = mapping;
		map<planStep*, planStep*> subBackmapping = backmapping;
		
		if (!mapping.count(psOne)){ // don't check again if we already did it
			mapping[psOne] = psOther;
			backmapping[psOther] = psOne;
			
			// run the DFS below this pair
			subRes = matchingDFS(one, other, psOne, psOther, subMapping, subBackmapping);
		}


		if (subRes){
			map<int,set<planStep*>> subOtherNextTasks = otherNextTasks;
			subOtherNextTasks[task].erase(psOther);
			
			// continue the extraction with the next task
			if (matchingGenerate(one,other,subMapping,subBackmapping,oneNextTasks,otherNextTasks,tasks))
				return true;
		}
	}

	return false; // did not find any valid matching
}


bool matchingDFS(searchNode* one, searchNode* other, planStep* oneStep, planStep* otherStep, map<planStep*, planStep*> mapping, map<planStep*, planStep*> backmapping){
	if (oneStep->numSuccessors != otherStep->numSuccessors) return false; // is not possible
	if (oneStep->numSuccessors == 0) return true; // no successors, matching OK

	
	// groups the successors into buckets according to their task labels
	map<int,set<planStep*>> oneNextTasks;
	map<int,set<planStep*>> otherNextTasks;
	unordered_set<int> tasks;

	for (int i = 0; i < oneStep->numSuccessors; i++){
		planStep* ps = oneStep->successorList[i];
		oneNextTasks[ps->task].insert(ps);
	}
	for (int i = 0; i < otherStep->numSuccessors; i++){
		planStep* ps = otherStep->successorList[i];
		otherNextTasks[ps->task].insert(ps);
	}
	
	for (int t : tasks) if (oneNextTasks[t].size() != otherNextTasks[t].size()) return false;

	return matchingGenerate(one,other,mapping,backmapping,oneNextTasks,otherNextTasks,tasks);
}


planStep* searchNodePSHead(searchNode * n){
	planStep* ps = new planStep();
	ps->numSuccessors = n->numAbstract + n->numPrimitive;
	ps->successorList = new planStep*[ps->numSuccessors];
	int pos = 0;
	for (int a = 0; a < n->numAbstract; a++)  ps->successorList[pos++] = n->unconstraintAbstract[a];
	for (int a = 0; a < n->numPrimitive; a++) ps->successorList[pos++] = n->unconstraintPrimitive[a];

	return ps;
}

bool matching(searchNode* one, searchNode* other){
	planStep* oneHead = searchNodePSHead(one);
	planStep* otherHead = searchNodePSHead(other);

	map<planStep*, planStep*> mapping;
	map<planStep*, planStep*> backmapping;
	mapping[oneHead] = otherHead;
	backmapping[otherHead] = oneHead;

	bool result = matchingDFS(one,other,oneHead,otherHead,mapping,backmapping);

	//delete oneHead;
	//delete otherHead;

	return result;
}





bool PriorityQueueSearch::insertVisi2(searchNode * n) {
    long lhash = 1;
    for(int i = 0; i < n->numContainedTasks; i++) {
        int numTasks = this->m->numTasks;
        int task = n->containedTasks[i];
        int count = n->containedTaskCount[i];
        cout << task << " " << count << endl;
        for(int j = 0; j < count; j++) {
            int p_index = j * numTasks + task;
            int p = getPrime(p_index);
            cout << "p: " << p << endl;
            lhash = lhash * p;
            lhash = lhash % 104729;
        }
    }
    int hash = (int) lhash;

    vector<uint64_t> ss = state2Int(n->state);
    auto it = visited2[ss].find(hash);
    if (it != visited2[ss].end()) {
        return false;
    }
    visited2[ss].insert(hash);
	return true;
}

bool insertVisi(searchNode * n){
	//set<planStep*> psp; map<planStep*,int> prec;
	//for (int a = 0; a < n->numAbstract; a++) dfsdfs(n->unconstraintAbstract[a], psp, prec);
	//for (int a = 0; a < n->numPrimitive; a++) dfsdfs(n->unconstraintPrimitive[a], psp, prec);
	//vector<int> seq;

	//while (psp.size()){
	//	for (planStep * ps : psp)
	//		if (prec[ps] == 0){
	//			seq.push_back(ps->task);
	//			psp.erase(ps);
	//			for (int ns = 0; ns < ps->numSuccessors; ns++)
	//				prec[ps->successorList[ns]]--;
	//			break;
	//		}
	//}


	//return true;

	std::clock_t before = std::clock();
	if (useTotalOrderMode){
		vector<int> seq;
		if (n->numPrimitive) to_dfs(n->unconstraintPrimitive[0],seq);
		if (n->numAbstract)  to_dfs(n->unconstraintAbstract[0], seq);

		A++;
		vector<uint64_t> ss = state2Int(n->state);
		auto it = visited[ss].find(seq);
		if (it != visited[ss].end()) {
			std::clock_t after = std::clock();
			time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
#ifdef SAVESEARCHSPACE 
			*stateSpaceFile << "duplicate " << n->searchNodeID << " " << it->second << endl;
#endif
			return false;
		}

#ifndef SAVESEARCHSPACE 
		visited[ss].insert(it,seq);
#else
		visited[ss][seq] = n->searchNodeID;
#endif

		B++;

		std::clock_t after = std::clock();
		time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
		return true;
	} else {
		A++;
		set<planStep*> psp;
		map<int,set<planStep*>> initial_Layers;
		set<pair<int,int>> pairs;
		for (int a = 0; a < n->numAbstract; a++) dfsdfs(n->unconstraintAbstract[a], 0, psp, pairs, initial_Layers);
		for (int a = 0; a < n->numPrimitive; a++) dfsdfs(n->unconstraintPrimitive[a], 0, psp, pairs, initial_Layers);
		
		psp.clear();
		map<int,set<planStep*>> layers;
		for (auto [d,pss] : initial_Layers){
			for (auto ps : pss)
				if (!psp.count(ps)){
					psp.insert(ps);
					layers[d].insert(ps);
				}
		}
		
		map<int,map<int,int>> layerCounts;
		for (auto [d,pss] : layers)
			for (auto ps : pss)
				layerCounts[d][ps->task]++;
		
		//cout << "Node:" << endl;
		//for (auto [a,b] : pairs) cout << setw(3) << a << " " << setw(3) << b << endl;
		//for (auto [d,pss] : layers){
		//	cout << "Layer: " << d;
		//	for (auto ps : pss) cout << " " << ps;
		//	cout << endl;
		//}
		
		vector<uint64_t> ss = state2Int(n->state);
		
		
		int i = 0;
		for (searchNode* other :  po_occ[{ss, layerCounts, pairs}]){
			/*			
			cout << "Checking ... #" << ++i << endl;
			n->printNode(std::cout);
			other->printNode(std::cout);
			
			fstream nf ("occurs_" + to_string(A) + "_n_" + to_string(i) + ".dot", fstream::out);
			fstream of ("occurs_" + to_string(A) + "_o_" + to_string(i) + ".dot", fstream::out);
			n->node2Dot(nf);
			other->node2Dot(of);
			*/
			
			bool result = matching(n,other);
			//cout << "Result: " << (result?"yes":"no") << endl;
			

			if (result) {
				std::clock_t after = std::clock();
				time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
				return false;
			}
		}
		po_occ[{ss, layerCounts, pairs}].push_back(n);	
		B++;
		
		std::clock_t after = std::clock();
		time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
		return true;
	}
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
	useTotalOrderMode = htn->isTotallyOrdered;
	timeval tp;
	gettimeofday(&tp, NULL);
	long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	long currentT;
	long lastOutput = startT;
	bool reachedTimeLimit = false;
	const int checkAfter = CHECKAFTER;
	int lastCheck = 0;

	this->m = htn;
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
#ifdef SAVESEARCHSPACE 
		*stateSpaceFile << "expanded " << n->searchNodeID << endl;
#endif
		assert(n != nullptr);
		fringe.pop();
#ifndef EARLYGOALTEST
		if (htn->isGoal(n)) {
#ifdef SAVESEARCHSPACE 
			*stateSpaceFile << "goal " << n->searchNodeID << endl;
#endif
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
#ifdef SAVESEARCHSPACE
					*stateSpaceFile << "nogoal " << n2->searchNodeID << endl;
#endif
					delete n2;
					continue;
				}

				hF->setHeuristicValue(n2, n, n->unconstraintPrimitive[i]->task);

#ifdef SAVESEARCHSPACE
				*stateSpaceFile << "heuristic " << n2->searchNodeID << " " << n2->heuristicValue << endl;
#endif
				
				assert(n2->goalReachable || (!htn->isGoal(n2))); // otherwise the heuristic is not save

#ifndef SAVESEARCHSPACE
				if (n2->goalReachable)
#endif
				{
#ifdef SAVESEARCHSPACE 
					n2->heuristicValue = 0;
#endif

#ifdef ASTAR
#ifdef ASTARAC
					n2->heuristicValue +=
						(n2->actionCosts / GASTARWEIGHT);
#else
					n2->heuristicValue +=
						(n2->modificationDepth / GASTARWEIGHT);
//							(n2->mixedModificationDepth / GASTARWEIGHT);
#endif
#endif
#ifdef EARLYGOALTEST
					if (htn->isGoal(n2)) {
#ifdef SAVESEARCHSPACE 
						*stateSpaceFile << "goal " << n2->searchNodeID << endl;
#endif
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

				}
#ifndef SAVESEARCHSPACE   
				else {
					delete n2;
				}
#endif
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
#ifdef SAVESEARCHSPACE
					*stateSpaceFile << "nogoal " << n2->searchNodeID << endl;
#endif
					delete n2;
					continue; // with next method
				}
				hF->setHeuristicValue(n2, n, decomposedStep, method);
				
#ifdef SAVESEARCHSPACE
				*stateSpaceFile << "heuristic " << n2->searchNodeID << " " << n2->heuristicValue << endl;
#endif
				
				assert(n2->goalReachable || (!htn->isGoal(n2))); // otherwise the heuristic is not save

#ifndef SAVESEARCHSPACE
				if (n2->goalReachable)
#endif
				{
#ifdef SAVESEARCHSPACE 
					n2->heuristicValue = 0;
#endif

#ifdef ASTAR
#ifdef ASTARAC
					n2->heuristicValue +=
						(n2->actionCosts / GASTARWEIGHT);
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

				}
#ifndef SAVESEARCHSPACE
				else {
					delete n2;
				}
#endif
			}
		}
		int allnodes = numSearchNodes + htn->numOneModActions + htn->numOneModMethods + htn->numEffLessProg;

		if (allnodes - lastCheck >= checkAfter) {
			lastCheck = allnodes;

			gettimeofday(&tp, NULL);
			currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

			if (((currentT - lastOutput) / 1000) > 0) {
				cout << setw(4) << int((currentT - startT) / 1000) << "s "
						<< "visitime " << setw(10) << fixed << setprecision(5) << time/1000 << "s"
					    << " generated nodes: " << setw(9) << allnodes
					   	<< " nodes/sec.: " << setw(7) << int(double(allnodes) / (currentT - startT) * 1000)
					    << " current heuristic: " << setw(5) << n->heuristicValue
					   	<< " mod.depth " << setw(5) << n->modificationDepth
					   	<< " inserts " << setw(9) << A
					   	<< " duplicates " << setw(9) << A-B
					   	<< " size " << setw(9) << B
						<< endl;
				lastOutput = currentT;
			}
			if ((timeLimit > 0) && ((currentT - startT) / 1000 > timeLimit)) {
				reachedTimeLimit = true;
				cout << "Reached time limit - stopping search." << endl;
				break;
			}
		}

		//delete n;
	}
	gettimeofday(&tp, NULL);
	currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	cout << "Search Results" << endl;
	cout << "- Search time " << double(currentT - startT) / 1000 << " seconds"	<< endl;
	cout << "- Visited list time " << time / 1000 <<  " seconds" << endl;
	cout << "- Visited list inserts " << A << endl;
	cout << "- Visited list pruned " << A-B << endl;
	cout << "- Visited list contains " << B << endl;
	cout << "- Generated " << (numSearchNodes + htn->numOneModActions + htn->numOneModMethods + htn->numEffLessProg) << " search nodes" << endl;
	cout << "  Calculated heuristic for " << numSearchNodes << " nodes" << endl;
	cout << "  One modifications " << (htn->numOneModActions + htn->numOneModMethods) << endl;
	cout << "  Effectless actions " << htn->numEffLessProg << endl;
	cout << "- including " << (htn->numOneModActions) << " one modification actions" << endl;
	cout << "- including " << (htn->numOneModMethods) << " one modification methods" << endl;
	cout << "- and       " << (htn->numEffLessProg) << " progressions of effectless actions" << endl;
	cout << "- Generated " << int(double(numSearchNodes) / (currentT - startT) * 1000) << " nodes per second" << endl;
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
		//cout << sol << endl;
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
