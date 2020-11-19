/*
 * PriorityQueueSearch.h
 *
 *  Created on: 10.09.2017
 *      Author: Daniel Höller
 */

#ifndef PRIORITYQUEUESEARCH_H_
#define PRIORITYQUEUESEARCH_H_
#include "../ProgressionNetwork.h"
#include "../heuristics/hhZero.h"
#include "../heuristics/rcHeuristics/hhRC.h"
#ifdef LMCOUNTHEURISTIC
#include "../heuristics/landmarks/hhLMCount.h"
#endif

#include <cassert>
#include <iomanip>
#include <sys/time.h>

namespace progression {


class PriorityQueueSearch {
public:
	PriorityQueueSearch();
	virtual ~PriorityQueueSearch();

template<class Heuristic, class VisitedList, class Fringe>
	void search(Model* htn, searchNode *tnI, int timeLimit, Heuristic & hF, VisitedList & visitedList, Fringe & fringe){
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
		cout << "- Using JAIR 2019 progression algorithm" << endl;
	
		if (optimzeSol) {
			cout << "- After first solution is found, search is continued until ";
			cout << "time limit to find better solution." << endl;
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
		
		// add initial search node to queue
		if (visitedList.insertVisi(tnI))
			fringe.push(tnI);
		assert(!fringe.empty());
	
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
	
			if (n->numAbstract == 0) {
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
			
					// check whether we have seen this one already	
					if (!visitedList.insertVisi(n2)){
						delete n2;
						continue;	
					}
	

					// compute the heuristic
					hF.setHeuristicValue(n2, n, n->unconstraintPrimitive[i]->task);
	
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
							fringe.push(n2);
	
					}
#ifndef SAVESEARCHSPACE   
					else {
						delete n2;
					}
#endif
				}
			}
			if (!continueSearch)
				break;
			if (n->numAbstract > 0) {
				int decomposedStep = rand() % n->numAbstract;
				int task = n->unconstraintAbstract[decomposedStep]->task;
				
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
				
					// check whether we have seen this one already	
					if (!visitedList.insertVisi(n2)){
						delete n2;
						continue;	
					}
	
					// compute the heuristic
					hF.setHeuristicValue(n2, n, decomposedStep, method);
					
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
							<< "visitime " << setw(7) << fixed << setprecision(2) << visitedList.time/1000 << "s"
						    << " generated nodes " << setw(9) << allnodes
						   	<< " nodes/sec " << setw(7) << int(double(allnodes) / (currentT - startT) * 1000)
						    << " cur h " << setw(4) << n->heuristicValue
						   	<< " mod.depth " << setw(4) << n->modificationDepth
						   	<< " inserts " << setw(9) << visitedList.attemptedInsertions
						   	<< " duplicates " << setw(9) << visitedList.attemptedInsertions - visitedList.uniqueInsertions
						   	<< " size " << setw(9) << visitedList.uniqueInsertions
						   	<< " hash fail " << setw(6) << visitedList.subHashCollision
							<< endl;
					lastOutput = currentT;
				}
				if ((timeLimit > 0) && ((currentT - startT) / 1000 > timeLimit)) {
					reachedTimeLimit = true;
					cout << "Reached time limit - stopping search." << endl;
					break;
				}
			}
	
			if (visitedList.canDeleteProcessedNodes)
				delete n;
		}
		gettimeofday(&tp, NULL);
		currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
		cout << "Search Results" << endl;
		cout << "- Search time " << double(currentT - startT) / 1000 << " seconds"	<< endl;
		cout << "- Visited list time " << visitedList.time / 1000 <<  " seconds" << endl;
		cout << "- Visited list inserts " << visitedList.attemptedInsertions << endl;
		cout << "- Visited list pruned " << visitedList.attemptedInsertions - visitedList.uniqueInsertions << endl;
		cout << "- Visited list contains " << visitedList.uniqueInsertions << endl;
		cout << "- Visited list hash collisions " << visitedList.subHashCollision << endl;
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
			cout << "- Total costs of actions: " << tnSol->actionCosts << endl;
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
		while (!fringe->empty()) {
			searchNode *n = fringe->top();
			fringe->pop();
			delete n;
		}
		delete tnSol;
#endif
	}



private:
	searchNode* handleNewSolution(searchNode* newSol, searchNode* globalSolPointer, long time);
	const bool optimzeSol = OPTIMIZEUNTILTIMELIMIT;
	int foundSols = 0;
	int solImproved = 0;
	long firstSolTime = 0;
	long bestSolTime = 0;

    };

} /* namespace progression */

#endif /* PRIORITYQUEUESEARCH_H_ */
