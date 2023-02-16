/*
 * ProgressionNetwork.cpp
 *
 *  Created on: 25.09.2017
 *      Author: Daniel Höller
 */

#include <stdlib.h>
#include <iomanip>
#include <algorithm>
#include "ProgressionNetwork.h"
#include "Model.h"

namespace progression {

#ifdef TRACESOLUTION
int currentSolutionStepInstanceNumber = 0;
#endif

#ifdef SAVESEARCHSPACE
int currentSearchNodeID = 0;
#endif 

////////////////////////////////
// solutionStep
////////////////////////////////
solutionStep::~solutionStep() {
	if (prev != nullptr) {
		prev->pointersToMe--;
		if (prev->pointersToMe == 0) {
			delete prev;
		}
	}
}
////////////////////////////////
// planStep
////////////////////////////////

bool planStep::operator==(const planStep &that) const {
	return (this->id == that.id);
}

planStep::~planStep() {
	for (int i = 0; i < numSuccessors; i++) {
		planStep* succ = successorList[i];
		succ->pointersToMe--;
		if (succ->pointersToMe == 0) {
			delete succ;
		}
	}
	delete[] successorList;
	delete[] reachableT;
#ifdef RCHEURISTIC
	delete[] goalFacts;
#endif
}

////////////////////////////////
// searchNode
////////////////////////////////

bool searchNode::operator<(searchNode other) const {
	return heuristicValue > other.heuristicValue;
}

searchNode::searchNode() {
	modificationDepth = -1;
	mixedModificationDepth = -1;
	unconstraintPrimitive = nullptr;
	unconstraintAbstract = nullptr;
	numAbstract = 0;
	numPrimitive = 0;
	solution = nullptr;
#ifdef SAVESEARCHSPACE
	searchNodeID = currentSearchNodeID++;
#endif
}

searchNode::~searchNode() {
	for (int i = 0; i < numAbstract; i++) {
		unconstraintAbstract[i]->pointersToMe--;
		if (unconstraintAbstract[i]->pointersToMe == 0) {
			delete unconstraintAbstract[i];
		}
	}
	for (int i = 0; i < numPrimitive; i++) {
		unconstraintPrimitive[i]->pointersToMe--;
		if (unconstraintPrimitive[i]->pointersToMe == 0) {
			delete unconstraintPrimitive[i];
		}
	}
	if (solution != nullptr) {
		solution->pointersToMe--;
		if (solution->pointersToMe == 0) {
			delete solution;
		}
	}
	delete[] unconstraintAbstract;
	delete[] unconstraintPrimitive;
	
	delete[] heuristicValue;

	delete[] containedTasks;
	delete[] containedTaskCount;

	// todo: need to destroy heuristic payload. To do so, I need to know the number of heuristics used
}


void searchNode::printDFS(planStep * s, map<planStep*,int> & psp, set<pair<planStep*,planStep*>> & orderpairs){
	if (psp.count(s)) return;
	int num = psp.size();
	psp[s] = num;
	for (int ns = 0; ns < s->numSuccessors; ns++){
		orderpairs.insert({s,s->successorList[ns]});
		this->printDFS(s->successorList[ns], psp, orderpairs);
	}
}


void searchNode::printNode(std::ostream & out){
	out << "Node: " << this << endl;
	for (int a = 0; a < this->numAbstract; a++)  cout << "\tUC A: " << this->unconstraintAbstract[a] << endl;
	for (int a = 0; a < this->numPrimitive; a++) cout << "\tUV P: " << this->unconstraintPrimitive[a] << endl;
	
	
	map<planStep*,int> psp;
	set<pair<planStep*,planStep*>> orderpairs;
	for (int a = 0; a < this->numAbstract; a++)  this->printDFS(this->unconstraintAbstract[a], psp, orderpairs);
	for (int a = 0; a < this->numPrimitive; a++) this->printDFS(this->unconstraintPrimitive[a],psp, orderpairs);

	// names
	map<int,planStep*> bpsp;
	for (auto [a,b] : psp) bpsp[b] = a;
	for (int i = 0; i < bpsp.size(); i++) out << "\t" << setw(2) << i << " " << bpsp[i] << " " << bpsp[i]->task << endl;

	// ordering
	for (auto [a,b] : orderpairs) out << "\t" << setw(2) << psp[a] << " < " << setw(2) << psp[b] << endl;
}

void searchNode::node2Dot(std::ostream & out){
	out << "digraph searchNode { "  << endl;
	map<planStep*,int> psp;
	set<pair<planStep*,planStep*>> orderpairs;
	for (int a = 0; a < this->numAbstract; a++)  this->printDFS(this->unconstraintAbstract[a], psp, orderpairs);
	for (int a = 0; a < this->numPrimitive; a++) this->printDFS(this->unconstraintPrimitive[a],psp, orderpairs);

	// names
	for (auto [a,b] : psp) out << "\t" << "n" << a << "[label=\"" << a->task <<  "\"];" << endl;

	// ordering
	for (auto [a,b] : orderpairs) out << "\tn" << a << " -> n" << b << ";" << endl;
	out << "}"; 
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

}
