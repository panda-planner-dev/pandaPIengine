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

const uint64_t max32BitP = 2147483647ULL;
const uint64_t over16BitP = 65537ULL;
const uint64_t tenMillionP = 16777213; // largest prime smaller than 16*1024*1014
const uint64_t oneMillionP = 1048573; // largest prime smaller than 1024*1014
const uint64_t tenThousandP = 16381; // largest prime smaller than 1024*16
const uint64_t tenThousandthP = 104729; // the 10.000th prime number


//#define DEBUG(x) do {  x; } while (0)


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


uint64_t hash_state_sequence(const vector<uint64_t> & state){
	uint64_t ret = 0;

	for (uint64_t x : state){
		uint64_t cur = x;
		for (int b = 0; b < 4; b++){
			uint64_t block = cur & ((1ULL << 16) - 1);
			cur = cur >> 16;
			ret = (ret * over16BitP) + block;
		}
	}

	return ret;
}



VisitedList::VisitedList(Model *m, bool _noVisitedCheck, bool _noReOpening, bool _taskHash, bool _taskSequenceHash, bool _topologicalOrdering, bool _orderPairs, bool _layers, bool _allowGIcheck, bool _allowedToUseParallelSequences) {
    this->htn = m;
	this->noVisitedCheck = _noVisitedCheck;
	this->noReopening = _noReOpening;
	this->GIcheck = _allowGIcheck;

	// auto detect properties of the problem
	this->useTotalOrderMode = this->htn->isTotallyOrdered;
    this->useSequencesMode = _allowedToUseParallelSequences && this->htn->isParallelSequences;
    this->canDeleteProcessedNodes = this->noVisitedCheck || this->useTotalOrderMode || this->useSequencesMode;

	this->taskHash = _taskHash;
	this->sequenceHash = _taskSequenceHash;
	this->topologicalOrdering = _topologicalOrdering;
	this->orderPairs = _orderPairs;
	this->layers = _layers;


	this->hashingP = tenMillionP; 
	this->stateTable = new hash_table(hashingP);
	if (!useSequencesMode)
		this->bitsNeededPerTask = sizeof(int)*8 -  __builtin_clz(m->numTasks - 1);
	else
		this->bitsNeededPerTask = sizeof(int)*8 -  __builtin_clz(m->numTasks); // one more ID is needed to separate parallel sequences

	cout << "Visited List configured" << endl;
	if (this->noVisitedCheck)
		cout << "- disabled" << endl;
	else {
		if (this->useTotalOrderMode)
			cout << "- mode: total order" << endl;
		else if (this->useSequencesMode)
			cout << "- mode: parallel sequences order" << endl;
		else
			cout << "- mode: partial order" << endl;
		
		cout << "- hashs to use: state";
		if (this->taskHash) cout << " task";
		if (this->sequenceHash) cout << " task-sequence";
		cout << endl;
	
		cout << "- memory information:";
		if (this->topologicalOrdering) cout << " topological ordering";
		if (this->orderPairs && ! (this->useTotalOrderMode || this->useSequencesMode)) cout << " order-pairs";
		if (this->layers && ! (this->useTotalOrderMode || this->useSequencesMode)) cout << " layer";
		cout << endl;
		
		
		if ((this->useTotalOrderMode || this->useSequencesMode) && !this->topologicalOrdering)
			cout << "- ATTENTION: pruning is " << color(RED,"INCOMPLETE") << endl;
	}
	cout << "- Visited list allows deletion of search nodes: " << ((this->canDeleteProcessedNodes) ? "true" : "false")
         << endl;
}



void dfsdfs(planStep *s, int depth, set<planStep *> &psp, set<pair<int, int>> &orderpairs,
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

	DEBUG(cout << "One Task " << task << endl);

    // possible partners
    for (planStep *psOther : otherNextTasks[task]) {
		DEBUG(cout << "\tOne Task " << task << endl);
		DEBUG(cout << "\tPartner " << psOther << " " << psOther->task << endl);
        
		if (mapping.count(psOne) && mapping[psOne] != psOther) continue;
        if (backmapping.count(psOther) && backmapping[psOther] != psOne) continue;
		DEBUG(cout << "\tOK" << endl);

        bool subRes = true;
        map<planStep *, planStep *> subMapping = mapping;
        map<planStep *, planStep *> subBackmapping = backmapping;

        if (!mapping.count(psOne)) { // don't check again if we already did it
            subMapping[psOne] = psOther;
            subBackmapping[psOther] = psOne;

            // run the DFS below this pair
            subRes = matchingDFS(one, other, psOne, psOther, subMapping, subBackmapping);
        }


        if (subRes) {
            map<int, set<planStep *>> subOtherNextTasks = otherNextTasks;
            subOtherNextTasks[task].erase(psOther);

            // continue the extraction with the next task
            if (matchingGenerate(one, other, subMapping, subBackmapping, oneNextTasks, subOtherNextTasks, tasks))
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

	DEBUG(cout << "Categorisation:" << endl);

    for (int i = 0; i < oneStep->numSuccessors; i++) {
        planStep *ps = oneStep->successorList[i];
        oneNextTasks[ps->task].insert(ps);
        tasks.insert(ps->task);
		DEBUG(cout << "\tone: " << ps->task << endl);
    }
    for (int i = 0; i < otherStep->numSuccessors; i++) {
        planStep *ps = otherStep->successorList[i];
        otherNextTasks[ps->task].insert(ps);
        tasks.insert(ps->task);
		DEBUG(cout << "\tother: " << ps->task << endl);
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

vector<int> VisitedList::topSort(searchNode *n) {
    vector<int> res;
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
        res.push_back(task);
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
uint64_t VisitedList::taskCountHash(searchNode * n){
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

uint64_t VisitedList::taskSequenceHash(vector<int> & tasks){
	uint64_t lhash = 0;
    for (int t : tasks) {
		// we need to use +1 here as the task sequence may contain the task htn->numTasks as a divider (for the parallel-seq case)
		lhash = (lhash * (htn->numTasks + 1)) % max32BitP;
		lhash += t;
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
    
	auto [sv,svpadding] = state2Int(exactBitString);

	vector<int> sequenceForHashing;

	if (topologicalOrdering || sequenceHash){
		if (useTotalOrderMode){
        	if (n->numPrimitive) to_dfs(n->unconstraintPrimitive[0], sequenceForHashing);
        	if (n->numAbstract) to_dfs(n->unconstraintAbstract[0], sequenceForHashing);
		} if (useSequencesMode) {
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
			bool first = true;
			for (vector<int> & sub : sequences){
				if (!first) sequenceForHashing.push_back(htn->numTasks);
				for (int & s : sub) sequenceForHashing.push_back(s);
				first = false;
			}
		} else {
			sequenceForHashing = topSort(n);
		}

		if (topologicalOrdering)
			for (int task : sequenceForHashing)
				for (int bit = 0; bit < bitsNeededPerTask; bit++)
					exactBitString.push_back(task & (1 << bit));
	}

	// only do this if we have a truly partially ordered instance
	if (!(useSequencesMode || useTotalOrderMode) && (orderPairs || layers)){
        set<planStep *> psp;
        vector<set<planStep *>> initial_Layers;
		set<pair<int, int>> pairs;
		// extract layering information
		for (int a = 0; a < n->numAbstract; a++)
			dfsdfs(n->unconstraintAbstract[a], 0, psp, pairs, initial_Layers);
        for (int a = 0; a < n->numPrimitive; a++)
			dfsdfs(n->unconstraintPrimitive[a], 0, psp, pairs, initial_Layers);

		// order pair hash if desired
		if (orderPairs){
			// write sorted pairs into list
			for (auto & [a,b] : pairs){
				//cout << "OP: " << a << " " << b << endl;
				
				for (int bit = 0; bit < bitsNeededPerTask; bit++)
					exactBitString.push_back(a & (1 << bit));
				for (int bit = 0; bit < bitsNeededPerTask; bit++)
					exactBitString.push_back(b & (1 << bit));
			}
			// add separator
			for (int bit = 0; bit < bitsNeededPerTask; bit++)
				exactBitString.push_back(htn->numTasks & (1 << bit));
		}

		// layer struture
		if (layers){
        	psp.clear();
        	vector<set<planStep *>> layers(initial_Layers.size());
        	for (int d = 0; d < initial_Layers.size(); d++) {
        	    for (auto ps : initial_Layers[d])
        	        if (!psp.count(ps)) {
        	            psp.insert(ps);
        	            layers[d].insert(ps);
        	        }
        	}

        	vector<map<int, int>> layerCounts(initial_Layers.size());
        	for (int d = 0; d < layers.size(); d++){
        	    for (auto ps : layers[d])
        	        layerCounts[d][ps->task]++;

			}

			bool first = true;
			for (map<int,int> & m : layerCounts){
				// push a separator
				if (!first)
					for (int bit = 0; bit < bitsNeededPerTask; bit++)
						exactBitString.push_back(htn->numTasks & (1 << bit));
				first = false;

				for (auto & [task, count] : m){
					// push the task
					for (int bit = 0; bit < bitsNeededPerTask; bit++)
						exactBitString.push_back(task & (1 << bit));
				
					int number = count;
					if (number > max_task_count)
						number = max_task_count;
					
					// determine length of number in bits
					int bits = (sizeof(int)*8 -  __builtin_clz(number)) - 1; // number will never be 0
					
					// push the lenght of the number
					for (int bit = 0; bit < number_of_bits_for_task_count; bit++)
						exactBitString.push_back(bits & (1 << bit));
					// puth the actual number
					for (int bit = 0; bit < bits; bit++)
						exactBitString.push_back(number & (1 << bit));
				}
			}
			// XXX if we ever add a hash after the layer hash, we must push *two* htn->numTasks in order to ensure a clean boundary
		}
	}

	// state access
    auto [accessVector,padding] = state2Int(exactBitString);
	
	// 2. STEP
	// compute the hashs
	uint64_t hash = hash_state_sequence(state2Int(n->state).first); // TODO double computation ...
	if (taskHash) hash = hash ^ taskCountHash(n);
	if (sequenceHash) hash = hash ^ taskSequenceHash(sequenceForHashing);


	// ACCESS Phase
	// access the hash hable
	compressed_sequence_trie ** stateEntry = (compressed_sequence_trie**) stateTable->get(hash);
	void ** payload;
	if (!*stateEntry)
		*stateEntry = new compressed_sequence_trie(accessVector,padding,payload);
	else {
		(*stateEntry)->insert(accessVector,padding,payload);
		subHashCollision++;
	}
	
	
	DEBUG(cout << "HASH     : " << hash << endl);
	DEBUG(cout << "HADR     : " << stateEntry << endl);
	DEBUG(cout << "ACCESS   :"; for (auto x : accessVector) cout << " " << bitset<64>(x); cout << " Padding: " << padding << endl);
	DEBUG(cout << "ADR      : " << payload << endl);
	DEBUG(cout << "READ     : " << *payload << endl);
	
	// 1. CASE
	// problem is totally ordered -- then we can use the total order mode
	if (useTotalOrderMode || useSequencesMode || !GIcheck) {
		// check if node was new
		bool returnValue = *payload == nullptr;

		if (noReopening){
			*payload = (void*) 1; // know the hash is known
		} else {
			int costOfInsertedNode = n->fValue + 1; // add 1 to distinguish f=0 from no search node at all.
			int costInTree = *(int*)payload;

			if (costInTree > costOfInsertedNode) // re-opening of the node
				returnValue = true;

			if (returnValue)
				*payload = (void*) costOfInsertedNode; // now the hash is known at the given cost
		}
		
		std::clock_t after = std::clock();
        this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
		if (returnValue) uniqueInsertions++;
	
		return returnValue;
	} else {
		vector<searchNode*> ** nodes = (vector<searchNode*> **) payload;
		if (*nodes == nullptr)
			*nodes = new vector<searchNode*>;
   
		searchNode* toRemove = nullptr;
		
		for (searchNode *other : **nodes) {
			bool result = matching(n, other);
			//cout << endl << endl << "Comp " << result << endl;
			//other->printNode(cout);
			if (result) {
				if (other->fValue > n->fValue){
					toRemove = other;
					continue;
				}
				std::clock_t after = std::clock();
				this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
				return false;
			}
		}

		if (toRemove != nullptr){
			vector<searchNode*> cpy_nodes = **nodes;
			(*nodes)->clear();
			
			for (searchNode *other : cpy_nodes)
				if (other != toRemove)
					(*nodes)->push_back(other);
		}
        
		(*nodes)->push_back(n);
		
		std::clock_t after = std::clock();
		this->time += 1000.0 * (after - before) / CLOCKS_PER_SEC;
		uniqueInsertions++;
		return true;
	}
}
