#include "VisitedList.h"
#include "../Model.h"
#include <cassert>
#include <chrono>
#include "primeNumbers.h"



VisitedList::VisitedList(Model * m){
	this->htn = m;
	this->useTotalOrderMode = this->htn->isTotallyOrdered;
}


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




/*
bool insertVisi2(searchNode * n) {
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

*/

bool VisitedList::insertVisi(searchNode * n){
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
			this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
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
		this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
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
				this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
				return false;
			}
		}
		po_occ[{ss, layerCounts, pairs}].push_back(n);	
		B++;
		
		std::clock_t after = std::clock();
		this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
		return true;
	}
}

