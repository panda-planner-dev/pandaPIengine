#include "VisitedList.h"
#include "Debug.h"
#include "Util.h"
#include "../Model.h"
#include <cassert>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include "primeNumbers.h"
#include <stack>
#include <list>
#include <bitset>
#include "../intDataStructures/IntPairHeap.h"
#include "../intDataStructures/CompressedSequenceSet.h"

const uint64_t max32BitP = 2147483647;
const uint64_t tenMillionP = 16777213; // largest prime smaller than 16*1024*1014
const uint64_t oneMillionP = 1048573; // largest prime smaller than 1024*1014
const uint64_t tenThousandP = 16381; // largest prime smaller than 1024*16
const uint64_t tenThousandthP = 104729; // the 10.000th prime number


pair<vector<uint64_t>,int> state2Int(vector<bool> &state) {
    int pos = 0;
    uint64_t cur = 0;
    vector<uint64_t> vec;
    for (bool b : state) {
        if (b)
            cur |= uint64_t(1) << pos;

        pos++;

		if (pos == 64) {
            vec.push_back(cur);
            pos = 0;
            cur = 0;
        }
    }

	int padding = 0;
    if (pos) {
		vec.push_back(cur);
		padding = 64 - pos;
	}

    return {vec,padding};
}



uint64_t hash_state(const vector<uint64_t> & v) {
	size_t r = 0;
	for (const uint64_t & x : v)
		r = r ^ x;
	return r;
}







VisitedList::VisitedList(Model *m, bool _noVisitedCheck, bool _taskHash, bool _topologicalOrdering) {
    this->htn = m;
	this->noVisitedCheck = _noVisitedCheck;

	// auto detect properties of the problem
	this->useTotalOrderMode = this->htn->isTotallyOrdered;
    this->useSequencesMode = this->htn->isParallelSequences;
    this->canDeleteProcessedNodes = this->noVisitedCheck || this->useTotalOrderMode || this->useSequencesMode;

	this->taskHash = _taskHash;
	this->topologicalOrdering = _topologicalOrdering;


	this->hashingP = tenMillionP; 
	this->stateTable = new hash_table(hashingP);
	this->bitsNeededPerTask = sizeof(int)*8 -  __builtin_clz(m->numTasks - 1);

	if (_noVisitedCheck)
    	this->canDeleteProcessedNodes = true;
#ifndef POVISI_EXACT
    this->canDeleteProcessedNodes = true;
#endif

	cout << "Visited List configured" << endl;
	if (this->noVisitedCheck)
		cout << "- disabled" << endl;
	else {
		cout << "- hashs to use: state";
		if (this->taskHash) cout << " task";
		cout << endl;
	
		cout << "- memory information:";
		if (this->topologicalOrdering) cout << " topological ordering";
		cout << endl;
		
		if ((this->useTotalOrderMode || this->useSequencesMode) && !this->topologicalOrdering)
			cout << "- ATTENTION: pruning is " << color(RED,"INCOMPLETE") << endl;
	}
	cout << "- Visited list allows deletion of search nodes: " << ((this->canDeleteProcessedNodes) ? "true" : "false")
         << endl;
}



void dfsdfs(planStep *s, int depth, set<planStep *> &psp, unordered_set<pair<int, int>> &orderpairs,
            vector<set<planStep *>> &layer) {
    if (psp.count(s)) return;
    psp.insert(s);
    if (layer.size() <= depth) layer.resize(depth + 1);
    layer[depth].insert(s);
    for (int ns = 0; ns < s->numSuccessors; ns++) {
        orderpairs.insert({s->task, s->successorList[ns]->task});
        dfsdfs(s->successorList[ns], depth + 1, psp, orderpairs, layer);
    }
}


void to_dfs(planStep *s, vector<int> &seq) {
    //cout << s->numSuccessors << endl;
    assert(s->numSuccessors <= 1);
    seq.push_back(s->task);
    if (s->numSuccessors == 0) return;
    to_dfs(s->successorList[0], seq);
}


bool matchingDFS(searchNode *one, searchNode *other, planStep *oneStep, planStep *otherStep,
                 map<planStep *, planStep *> mapping, map<planStep *, planStep *> backmapping);


void getSortedTasks(planStep **network, int size, int *succ);

bool matchingGenerate(
        searchNode *one, searchNode *other,
        map<planStep *, planStep *> &mapping, map<planStep *, planStep *> &backmapping,
        map<int, set<planStep *>> oneNextTasks,
        map<int, set<planStep *>> otherNextTasks,
        unordered_set<int> tasks
) {
    if (tasks.size() == 0)
        return true; // done, all children

    if (oneNextTasks[*tasks.begin()].size() == 0) {
        tasks.erase(*tasks.begin());
        return matchingGenerate(one, other, mapping, backmapping, oneNextTasks, otherNextTasks, tasks);
    }

    int task = *tasks.begin();

    planStep *psOne = *oneNextTasks[task].begin();
    oneNextTasks[task].erase(psOne);

    // possible partners
    for (planStep *psOther : otherNextTasks[task]) {
        if (mapping.count(psOne) && mapping[psOne] != psOther) continue;
        if (backmapping.count(psOther) && backmapping[psOther] != psOne) continue;

        bool subRes = true;
        map<planStep *, planStep *> subMapping = mapping;
        map<planStep *, planStep *> subBackmapping = backmapping;

        if (!mapping.count(psOne)) { // don't check again if we already did it
            mapping[psOne] = psOther;
            backmapping[psOther] = psOne;

            // run the DFS below this pair
            subRes = matchingDFS(one, other, psOne, psOther, subMapping, subBackmapping);
        }


        if (subRes) {
            map<int, set<planStep *>> subOtherNextTasks = otherNextTasks;
            subOtherNextTasks[task].erase(psOther);

            // continue the extraction with the next task
            if (matchingGenerate(one, other, subMapping, subBackmapping, oneNextTasks, otherNextTasks, tasks))
                return true;
        }
    }

    return false; // did not find any valid matching
}

bool matchingDFS(searchNode *one, searchNode *other, planStep *oneStep, planStep *otherStep,
                 map<planStep *, planStep *> mapping, map<planStep *, planStep *> backmapping) {
    if (oneStep->numSuccessors != otherStep->numSuccessors) return false; // is not possible
    if (oneStep->numSuccessors == 0) return true; // no successors, matching OK


    // groups the successors into buckets according to their task labels
    map<int, set<planStep *>> oneNextTasks;
    map<int, set<planStep *>> otherNextTasks;
    unordered_set<int> tasks;

    for (int i = 0; i < oneStep->numSuccessors; i++) {
        planStep *ps = oneStep->successorList[i];
        oneNextTasks[ps->task].insert(ps);
        tasks.insert(ps->task);
    }
    for (int i = 0; i < otherStep->numSuccessors; i++) {
        planStep *ps = otherStep->successorList[i];
        otherNextTasks[ps->task].insert(ps);
        tasks.insert(ps->task);
    }

    for (int t : tasks) if (oneNextTasks[t].size() != otherNextTasks[t].size()) return false;

    return matchingGenerate(one, other, mapping, backmapping, oneNextTasks, otherNextTasks, tasks);
}


planStep *searchNodePSHead(searchNode *n) {
    planStep *ps = new planStep();
    ps->numSuccessors = n->numAbstract + n->numPrimitive;
    ps->successorList = new planStep *[ps->numSuccessors];
    int pos = 0;
    for (int a = 0; a < n->numAbstract; a++) ps->successorList[pos++] = n->unconstraintAbstract[a];
    for (int a = 0; a < n->numPrimitive; a++) ps->successorList[pos++] = n->unconstraintPrimitive[a];

    return ps;
}

bool matching(searchNode *one, searchNode *other) {
    planStep *oneHead = searchNodePSHead(one);
    planStep *otherHead = searchNodePSHead(other);

    map<planStep *, planStep *> mapping;
    map<planStep *, planStep *> backmapping;
    mapping[oneHead] = otherHead;
    backmapping[otherHead] = oneHead;

    bool result = matchingDFS(one, other, oneHead, otherHead, mapping, backmapping);

    //delete oneHead;
    //delete otherHead;

    return result;
}

vector<int> *VisitedList::topSort(searchNode *n) {
    vector<int> *res = new vector<int>;
    IntPairHeap<int> unconstrained(50);
    map<int, set<int>> successors;
    map<int, set<int>> predecessors;
    map<int, int> idToTask;

    list<planStep *> todo;
    for (int i = 0; i < n->numPrimitive; i++) {
        int id = n->unconstraintPrimitive[i]->id;
        int task = n->unconstraintPrimitive[i]->task;
        unconstrained.add(task, id);
        todo.push_back(n->unconstraintPrimitive[i]);
    }
    for (int i = 0; i < n->numAbstract; i++) {
        int id = n->unconstraintAbstract[i]->id;
        int task = n->unconstraintAbstract[i]->task;
        unconstrained.add(task, id);
        todo.push_back(n->unconstraintAbstract[i]);
    }
    set<int> done;
    while (!todo.empty()) {
        planStep *ps = todo.front();
        todo.pop_front();
        if (done.find(ps->id) != done.end()) {
            continue;
        }
        done.insert(ps->id);
        idToTask.insert({ps->id, ps->task});
        for (int i = 0; i < ps->numSuccessors; i++) {
            planStep *succ = ps->successorList[i];
            todo.push_back(succ);
            successors[ps->id].insert(succ->id);
            predecessors[succ->id].insert(ps->id);
        }
    }

    while (!unconstrained.isEmpty()) {
        int task = unconstrained.topKey();
        int id = unconstrained.topVal();
        unconstrained.pop();
        res->push_back(task);
        set<int> &succs = successors[id];
        for (int succ : succs) {
            predecessors[succ].erase(id);
            if (predecessors[succ].empty()) {
                unconstrained.add(idToTask[succ], succ);
            }
        }
    }
    return res;
}

uint32_t VisitedList::getHash(vector<int> *seq) {
    const uint64_t bigP = max32BitP;
    uint64_t hash = 0;
    for (int i = 0; i < seq->size(); i++) {
        hash *= htn->numTasks;
        hash += seq->at(i);
        hash %= bigP;
    }
    hash += INT32_MIN;
    return (uint32_t) hash;
}



// computes the task count hash
// completely ignores order of tasks, but still considers how often specific types of tasks occur
size_t VisitedList::taskCountHash(searchNode * n){
	uint64_t lhash = 1;
    for (int i = 0; i < n->numContainedTasks; i++) {
        uint64_t numTasks = this->htn->numTasks;
        uint64_t task = n->containedTasks[i];
        uint64_t count = n->containedTaskCount[i];
        DEBUG(cout << task << " " << count << endl);
        for (unsigned int j = 0; j < count; j++) {
            unsigned int p_index = j * numTasks + task;
            unsigned int p = getPrime(p_index);
            DEBUG(cout << "p: " << p << endl);
            lhash = lhash * p;
            lhash = lhash % tenThousandthP;
        }
    }
    return lhash;
}



bool VisitedList::insertVisi(searchNode *n) {
    if (noVisitedCheck) return true;

    std::clock_t before = std::clock();
    attemptedInsertions++;

	// 1. STEP
	// compute the exact information
	vector<bool> exactBitString = n->state;
	// add everything we choose to append to the bitstring

	if (topologicalOrdering){
		vector<int> seq;
		if (useTotalOrderMode){
        	if (n->numPrimitive) to_dfs(n->unconstraintPrimitive[0], seq);
        	if (n->numAbstract) to_dfs(n->unconstraintAbstract[0], seq);
		} else {
			assert(false); // XXX idea of ICAPS reviewer
		}

		for (int task : seq)
			for (int bit = 0; bit < bitsNeededPerTask; bit++)
				exactBitString.push_back(task & (1 << bit));
	}



	// state access
    auto [vector,padding] = state2Int(exactBitString);

	// 2. STEP
	// compute the hashs
	uint64_t hash = hash_state(state2Int(n->state).first); // TODO double computation ...
	if (taskHash)
		hash = hash ^ taskCountHash(n);

	// ACCESS Phase
	// access the hash hable
	compressed_sequence_trie ** stateEntry = (compressed_sequence_trie**) stateTable->get(hash);
	void ** payload;
	if (!*stateEntry)
		*stateEntry = new compressed_sequence_trie(vector,padding,payload);
	else {
		(*stateEntry)->insert(vector,padding,payload);
	}

	// 1. CASE
	// problem is totally ordered -- then we can use the total order mode
	if (useTotalOrderMode) {
		// check if node was new
		DEBUG(cout << "HASH     : " << hash << endl);
		DEBUG(cout << "READ     : " << *payload << endl);
		bool returnValue = *payload == nullptr;
		*payload = (void*) 1; // now the hash is known
		
		std::clock_t after = std::clock();
        this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
		if (returnValue) uniqueInsertions++;
		return returnValue;
	}


	return false;

/*

#if (TOVISI == TOVISI_PRIM) || (TOVISI == TOVISI_PRIM_EXACT)
#endif


    if (useTotalOrderMode) {
#if (TOVISI == TOVISI_SEQ) || (TOVISI == TOVISI_PRIM_EXACT)
        vector<int> seq;
        if (n->numPrimitive) to_dfs(n->unconstraintPrimitive[0], seq);
        if (n->numAbstract) to_dfs(n->unconstraintAbstract[0], seq);
#endif

#if (TOVISI == TOVISI_SEQ)
        auto it = visited[ss].find(seq);
#elif (TOVISI == TOVISI_PRIM) || (TOVISI == TOVISI_PRIM_EXACT)
        auto it = visited[ss].find(hash);
#endif

#if (TOVISI == TOVISI_SEQ) || (TOVISI == TOVISI_PRIM)
        if (it != visited[ss].end()) {
#elif (TOVISI == TOVISI_PRIM_EXACT)
        if (it != visited[ss].end() && it->second.count(seq)) {
#endif
            std::clock_t after = std::clock();
            this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
#ifndef    VISITEDONLYSTATISTICS
            return false;
#else
            return true;
#endif
        }


#if (TOVISI == TOVISI_SEQ)
        visited[ss].insert(it,seq);
#elif (TOVISI == TOVISI_PRIM)
        visited[ss].insert(it,hash);
#elif (TOVISI == TOVISI_PRIM_EXACT)
        if (visited[ss][hash].size() > 0) subHashCollision++;
        visited[ss][hash].insert(seq);
#endif

        uniqueInsertions++;

        std::clock_t after = std::clock();
        this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
        return true;
    } else {
        po_hash_tuple access;

        // state
        get<0>(access) = ss;


#if defined(POVISI_ORDERPAIRS) || defined(POVISI_LAYERS)
        set<planStep *> psp;
        vector<set<planStep *>> initial_Layers;
        unordered_set<pair<int, int>> pairs;
        for (int a = 0; a < n->numAbstract; a++) dfsdfs(n->unconstraintAbstract[a], 0, psp, pairs, initial_Layers);
        for (int a = 0; a < n->numPrimitive; a++) dfsdfs(n->unconstraintPrimitive[a], 0, psp, pairs, initial_Layers);
#endif

#ifdef POVISI_LAYERS
        psp.clear();
        vector<set<planStep *>> layers(initial_Layers.size());
        for (int d = 0; d < initial_Layers.size(); d++) {
            for (auto ps : initial_Layers[d])
                if (!psp.count(ps)) {
                    psp.insert(ps);
                    layers[d].insert(ps);
                }
        }

        vector<unordered_map<int, int>> layerCounts(initial_Layers.size());
        for (int d = 0; d < layers.size(); d++)
            for (auto ps : layers[d])
                layerCounts[d][ps->task]++;
#endif

#define POS1 1
#ifdef POVISI_HASH
        get<POS1>(access) = hash;
#define POS2 (POS1+1)
#else
#define POS2 POS1
#endif


#ifdef POVISI_LAYERS
        get<POS2>(access) = layerCounts;
#define POS3 (POS2+1)
#else
#define POS3 POS2
#endif


#ifdef POVISI_ORDERPAIRS
        get<POS3>(access) = pairs;
#define POS4 (POS3+1)
#else
#define POS4 POS3
#endif

        //cout << "Node:" << endl;
        //for (auto [a,b] : pairs) cout << setw(3) << a << " " << setw(3) << b << endl;
        //for (auto [d,pss] : layers){
        //	cout << "Layer: " << d;
        //	for (auto ps : pss) cout << " " << ps;
        //	cout << endl;
        //}

        ////////////////////////////////
        // approximate test
#ifndef POVISI_EXACT
        if (po_occ.count(access)){
            std::clock_t after = std::clock();
            this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
#ifndef	VISITEDONLYSTATISTICS
            return false;
#else
            return true;
#endif
        }
        // insert into the visited list as this one is new
        po_occ.insert(access);
#endif

        ////////////////////////////////
        // exact test
#ifdef POVISI_EXACT
        if (useSequencesMode) {
            // get sequences
            vector<vector<int>> sequences;
            for (int a = 0; a < n->numAbstract; a++) {
                vector<int> seq;
                to_dfs(n->unconstraintAbstract[a], seq);
                sequences.push_back(seq);
            }
            for (int a = 0; a < n->numPrimitive; a++) {
                vector<int> seq;
                to_dfs(n->unconstraintPrimitive[a], seq);
                sequences.push_back(seq);
            }
            // sort them to be unique
            sort(sequences.begin(), sequences.end());

            auto &dups = po_seq_occ[access];
            if (dups.count(sequences)) {
                std::clock_t after = std::clock();
                this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
#ifndef    VISITEDONLYSTATISTICS
                return false;
#else
                return true;
#endif
            }

            // insert into the visited list as this one is new
            if (dups.size() > 0) subHashCollision++;
            dups.insert(sequences);
        } else {
            auto &dups = po_occ[access];
            for (searchNode *other : dups) {
                bool result = matching(n, other);
                if (result) {
                    std::clock_t after = std::clock();
                    this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
#ifndef    VISITEDONLYSTATISTICS
                    return false;
#else
                    return true;
#endif
                }
            }

            // insert into the visited list as this one is new
            if (dups.size() > 0) subHashCollision++;
            dups.push_back(n);
        }
#endif


        uniqueInsertions++;
        std::clock_t after = std::clock();
        this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
        return true;
    }
*/
}
