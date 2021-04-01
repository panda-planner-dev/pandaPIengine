#include "VisitedList.h"
#include "Debug.h"
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


vector<uint64_t> state2Int(vector<bool> &state) {
    int pos = 0;
    uint64_t cur = 0;
    vector<uint64_t> vec;
    for (bool b : state) {
        if (pos == 64) {
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


inline uint64_t bitmask_ignore_first(int bits){
	return ~((uint64_t(1) << bits)-1);
}

inline uint64_t bitmask_ignore_last(int bits){
	return (uint64_t(1) << (64-bits))-1;
}

inline uint64_t bitmask_bit(int bit){
	return uint64_t(1) << bit;
}

void sequence_trie::print_node(int indent){
	for(int i = 0; i < indent; i++) cout << " ";
	cout << blocks << " blocks; block position " << myFirstBlock << "; ignore first " << ignoreBitsFirst << "; ignore last " << ignoreBitsLast << "; payload " << payload << endl;

	for(int i = 0; i < indent; i++) cout << " ";
	cout << "M " << bitset<64>(bitmask_ignore_first(ignoreBitsFirst)) << endl;
	for (int b = 0; b < blocks; b++){
		for(int i = 0; i < indent; i++) cout << " ";
		cout << "V " << bitset<64>(value[b]) << endl;
	}
	for(int i = 0; i < indent; i++) cout << " ";
	cout << "M " << bitset<64>(bitmask_ignore_last(ignoreBitsLast)) << endl;
}

void sequence_trie::print_tree(int indent){
	print_node(indent);
	if (one){
		for(int i = 0; i < indent; i++) cout << " ";
		cout << "child one";
		one->print_tree(indent+2);
	}
	if (zero){
		for(int i = 0; i < indent; i++) cout << " ";
		cout << "child zero";
		zero->print_tree(indent+2);
	}
}

sequence_trie::sequence_trie(){
	payload = 0;
	blocks = 0;
	ignoreBitsFirst = 0;
	ignoreBitsLast = 0;
	myFirstBlock = 0;
	zero = nullptr;
	one = nullptr;
}

sequence_trie::~sequence_trie(){
	free(value);
	delete zero;
	delete one;
}

sequence_trie::sequence_trie(const vector<uint64_t> & sequence, int paddingBits, uint64_t* & p) : sequence_trie(){
	blocks = sequence.size();
	p = &payload;
	myFirstBlock = 0;
	ignoreBitsFirst = 0;
	ignoreBitsLast = paddingBits;
	zero = one = nullptr;
	// copy sequence into value	
	value = (uint64_t*) calloc(sequence.size(), sizeof(uint64_t));
	for (int i = 0; i < sequence.size(); i++)
		value[i] = sequence[i];
}


sequence_trie::sequence_trie(const vector<uint64_t> & sequence, int startingBlock, int startingBit, int paddingBits, uint64_t* & p) : sequence_trie(){
	blocks = sequence.size() - startingBlock;
	p = &payload;
	myFirstBlock = 0;
	ignoreBitsFirst = startingBit;
	ignoreBitsLast = paddingBits;
	zero = one = nullptr;
	// copy sequence into value	
	value = (uint64_t*) calloc(blocks, sizeof(uint64_t));
	for (int i = 0; i < blocks; i++)
		value[i] = sequence[i + startingBlock];

	value[0] = value[0] & bitmask_ignore_first(startingBit);
}

void sequence_trie::split_me_at(int block, int bit){
	DEBUG(cout << "Starting split of block " << block << " @ bit " << bit << endl);
	
	sequence_trie * child = new sequence_trie();
	// child gets all blocks from (inclusive) block to blocks
	child->blocks = blocks - block; DEBUG(cout << "\tchild receives " << child->blocks << " blocks " << endl);
	assert(child->blocks);
	child->myFirstBlock = myFirstBlock + block;
	child->value = (uint64_t*) calloc(child->blocks, sizeof(uint64_t));
	for (int i = 0; i < child->blocks; i++)
		child->value[i] = value[i + block]; // copy blocks to child	
	// remove the bits before bit from the child's first block
	child->value[0] = child->value[0] & bitmask_ignore_first(bit);
	// child inherits my current last padding
	child->ignoreBitsLast = ignoreBitsLast;
	// child ignores everything up to this bit
	child->ignoreBitsFirst = bit;
	
	child->payload = payload;
	payload = 0;
	child->zero = zero;
	child->one = one;
	
	// the child is now my child
	one = zero = nullptr;
	if (child->value[0] & bitmask_bit(bit))
		one = child;
	else
		zero = child;

	// curb myself
	ignoreBitsLast = 64 - bit;
	if (ignoreBitsLast == 64) ignoreBitsLast = 0;
	blocks = (bit != 0) ? block + 1 : block;
	
	// I might end up with no bits at all (edge cases ...)
	if (blocks){
		uint64_t* newValue = (uint64_t*) calloc(blocks, sizeof(uint64_t));
		for (int i = 0; i < blocks; i++)
			newValue[i] = value[i];
		// remove last bits
		newValue[blocks-1] = newValue[blocks-1] & bitmask_ignore_last(ignoreBitsLast);

		free(value);
		value = newValue;
	}
	TEST(check_integrity());
}


void sequence_trie::test_and_insert(const vector<uint64_t> & sequence, int paddingBits, uint64_t* & p){
	// all of the padding bits must actually be zero, else the testing of the difference breaks
	assert((sequence.back() & bitmask_ignore_first(64-paddingBits)) == 0);

	DEBUG(cout << endl << endl << "Calling test and insert; first block " << myFirstBlock << endl; print_node(2));

	// find first differing uint64_t
	int block_with_first_difference = -1;
	for (int i = 0; i < blocks; i++){
		if (myFirstBlock + i >= sequence.size()) break;
		uint64_t input_test = sequence[myFirstBlock + i];
		DEBUG(cout << "testing block " << i << " (globally " << myFirstBlock + i <<  ": " << bitset<64>(input_test) << ")" << endl);
		if (i == 0) input_test = input_test & bitmask_ignore_first(ignoreBitsFirst);
		if (i == blocks-1) input_test = input_test & bitmask_ignore_last(ignoreBitsLast);
		uint64_t value_test = value[i];
		// if the input sequence is shorter then my value, I can only compare up to this length
		if (myFirstBlock + i ==  sequence.size() - 1)
			value_test = value_test & bitmask_ignore_last(paddingBits);
		
		DEBUG(cout << "\tafter bitmask: " << bitset<64>(input_test) << endl);
		if (input_test != value_test){
			block_with_first_difference = i;
			break;
		}
	}

	DEBUG(cout << "first block with difference: " << block_with_first_difference << endl);

	int splitBit = -1;

	// no difference found
	if (block_with_first_difference == -1){
		// check if the end of the overall sequence is the end of this value
		if (myFirstBlock + blocks == sequence.size()){
			DEBUG(cout << "\tlast block" << endl);
			// check padding
			if (ignoreBitsLast == paddingBits){
				DEBUG(cout << "\t$$$ found matching" << endl);
				// sequence is already contained
				p = &payload;
				TEST(check_integrity());
				return;
			} else if (ignoreBitsLast > paddingBits) {
				DEBUG(cout << "\tNew sequence ends beyond (last block)." << endl);
				// continue with child
				uint64_t next_bit_value = sequence.back() & bitmask_bit(64 - ignoreBitsLast);
				if (next_bit_value){
					if (one)
						one->test_and_insert(sequence, paddingBits, p);
					else
						one = new sequence_trie(sequence, sequence.size() - 1, 64 - ignoreBitsLast, paddingBits, p);
				} else {
					if (zero)
						zero->test_and_insert(sequence, paddingBits, p);
					else
						zero = new sequence_trie(sequence, sequence.size() - 1, 64 - ignoreBitsLast, paddingBits, p);
				}
				TEST(check_integrity());
				return;
			} else {
				DEBUG(cout << "\tNew sequence ends within." << endl);
				assert(ignoreBitsLast < paddingBits); // need to split
				block_with_first_difference = blocks - 1;
				splitBit = 64 - paddingBits;
				// split
				split_me_at(block_with_first_difference,splitBit);
				p = &payload;
				TEST(check_integrity());
				return;
			}
		} else if (myFirstBlock + blocks < sequence.size()) {
			DEBUG(cout << "\tNew sequence ends beyond." << endl);
			// this segment is fully equal and there is a next block
			// continue with child
			int next_bit = 64 - ignoreBitsLast;
			if (next_bit == 64) next_bit = 0;
			int nextBlock = myFirstBlock + blocks - ((next_bit)?1:0);
			uint64_t next_bit_value = sequence[nextBlock] & bitmask_bit(next_bit);

			if (next_bit_value){
				DEBUG(cout << "\tNext bit is one." << endl);
				if (one){
					DEBUG(cout << "\tInsert." << endl);
					one->test_and_insert(sequence, paddingBits, p);
				} else {
					DEBUG(cout << "\tCreate." << endl);
					one = new sequence_trie(sequence, nextBlock, next_bit, paddingBits, p);
				}
			} else {
				DEBUG(cout << "\tNext bit is zero." << endl);
				if (zero) {
					DEBUG(cout << "\tInsert." << endl);
					zero->test_and_insert(sequence, paddingBits, p);
				} else {
					DEBUG(cout << "\tCreate." << endl);
					zero = new sequence_trie(sequence, nextBlock, next_bit, paddingBits, p);
				}
			}
			TEST(check_integrity());
			return;
		} else {
			// this segment captures more than the segment, but is fully equal
			// --> just split
			DEBUG(cout << "\tNew sequence ends within." << endl);
			block_with_first_difference = sequence.size() - myFirstBlock - 1;
			splitBit = 64 - paddingBits;
			// split
			split_me_at(block_with_first_difference,splitBit);
			p = &payload;
			TEST(check_integrity());
			return;
		}
	}

	uint64_t input_test = sequence[myFirstBlock + block_with_first_difference];
	uint64_t myValue = value[block_with_first_difference];
	if (block_with_first_difference == 0) input_test = input_test & bitmask_ignore_first(ignoreBitsFirst);
	// find first position with difference in this block
	for (int i = 0; i < 64; i++)
		if ((input_test & bitmask_bit(i)) != (myValue & bitmask_bit(i))){
			splitBit = i;
			break;
		}
	
	DEBUG(
		cout << "Split Block: " << block_with_first_difference << endl;
		cout << "Split Bit  : " << splitBit << endl;
			);

	// split me	
	split_me_at(block_with_first_difference,splitBit);

	if (sequence[myFirstBlock + block_with_first_difference] & bitmask_bit(splitBit))
		one = new sequence_trie(sequence,myFirstBlock + block_with_first_difference, splitBit, paddingBits, p);
	else
		zero = new sequence_trie(sequence,myFirstBlock + block_with_first_difference, splitBit, paddingBits, p);

	TEST(check_integrity());
	// split
	return;
}

void sequence_trie::check_integrity(){
	assert(ignoreBitsLast != 64);
	if (myFirstBlock || ignoreBitsFirst){
		assert(ignoreBitsFirst != 64);
		// must have at least one bit
		if (blocks == 1) assert(ignoreBitsFirst + ignoreBitsLast < 64);
	}

	if (one) one->check_integrity();
	if (zero) zero->check_integrity();
}

void printMemory();

void speed_test(){
	cout << "SIZE " << sizeof(sequence_trie) << endl;

	int numbers = 200000;

	setDebugMode(false);

	srand(42);
	std::clock_t beforeMap = std::clock();
	map<pair<vector<uint64_t>,int>,int> data;
	for (int j = 0 ; j < numbers; j++){
		vector<uint64_t> vec;
		int len = rand() % 100;
		for (int i = 0; i < len; i++)
			vec.push_back(uint64_t(rand()) * uint64_t(rand()));
		int padding = rand() % 64;
		vec.push_back((uint64_t(rand()) * uint64_t(rand())) & bitmask_ignore_last(padding));
		
		data[make_pair(vec,padding)] = j+1;
	}
	printMemory();
	std::clock_t afterMap = std::clock();
	double map_time = 1000.0 * (afterMap - beforeMap) / CLOCKS_PER_SEC;
	cout << "Map took: " << setprecision(3) << fixed << map_time << " ms" << endl;

	srand(42);

	printMemory();
	sequence_trie * t = nullptr; 
	std::clock_t beforeTrie = std::clock();
	for (int j = 0 ; j < numbers; j++){
		vector<uint64_t> vec;
		int len = rand() % 100;
		for (int i = 0; i < len; i++)
			vec.push_back(uint64_t(rand()) * uint64_t(rand()));
		int padding = rand() % 64;
		vec.push_back((uint64_t(rand()) * uint64_t(rand())) & bitmask_ignore_last(padding));

		uint64_t *pay;
		if (!t)
			t = new sequence_trie(vec,padding,pay);
		else {
			t->test_and_insert(vec,padding,pay);
		}
		
		*pay = j+1;
	}
	
	std::clock_t afterTrie = std::clock();
	double trie_time = 1000.0 * (afterTrie - beforeTrie) / CLOCKS_PER_SEC;
	cout << "Trie took: " << setprecision(3) << fixed << trie_time << " ms" << endl;
	printMemory();
	}

void test(){
	setDebugMode(false);
	srand(42);
	sequence_trie * t = nullptr; 

	map<pair<vector<uint64_t>,int>,int> data;

	for (int j = 0 ; j < 10000; j++){
		vector<uint64_t> vec;
		int len = rand() % 100;
		for (int i = 0; i < len; i++)
			vec.push_back(uint64_t(rand()) * uint64_t(rand()));
		int padding = rand() % 64;
		vec.push_back((uint64_t(rand()) * uint64_t(rand())) & bitmask_ignore_last(padding));
		
		DEBUG(cout << endl << endl << endl);
		cout << "pushing #" << j << " "; for (uint64_t v : vec) cout << v << ","; cout << " pad: " << padding; 

		uint64_t *pay;
		if (!t)
			t = new sequence_trie(vec,padding,pay);
		else {
			DEBUG(t->print_tree(0));
			t->test_and_insert(vec,padding,pay);
			DEBUG(cout << "==============================================================================" << endl);
			DEBUG(t->print_tree(0));
		}
		cout << " @ " << pay << endl;
		if (data.count(make_pair(vec,padding)) == 0)
			assert(*pay == 0);
		else
			assert(*pay == data[make_pair(vec,padding)]);
	
		*pay = j + 1;
		data[make_pair(vec,padding)] = *pay;

		// check integrity of the tree in every step
		for (auto [key,value] : data){
			uint64_t *pay = nullptr;
			auto [vec,padding] = key;
			t->test_and_insert(vec,padding,pay);
			DEBUG(cout << "checking "; for (uint64_t v : vec) cout << v << ","; cout << " pad: " << padding << " @ " << pay << endl);
			assert(*pay == value);
		}
	}




	delete t;
	
	
	
	//vector<bool> vec = {0,1,1,1,0,1};
	//vector<bool> vec2 = {0,1,0,1,0,1};
	//vector<uint64_t> state = state2Int(vec); state.push_back(55); state.push_back(100);
	//vector<uint64_t> state2 = state2Int(vec);
	//vector<uint64_t> state3 = state2Int(vec2);
	//cout << "Bitset 1: " << bitset<64>(state[0]) << endl;
	//cout << "Bitset 2: " << bitset<64>(state2[0]) << endl;
	//cout << "Bitset 3: " << bitset<64>(state3[0]) << endl;
	////cout << "Bitset M: " << bitset<64>(bitmask_ignore_last(5)) << endl;
	//
	//int* pay;
	//sequence_trie * t = new sequence_trie(state2,55,pay); *pay = 10;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->test_and_insert(state3,55,pay); *pay = 20;
	//cout << endl;
	//t->print_tree(0);

	//int* pay;
	//sequence_trie * t = new sequence_trie(state,55,pay); *pay = 10;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->test_and_insert(state,57,pay); *pay = 20;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->test_and_insert(state2,58,pay); *pay = 30;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//cout << "PAY " << *pay << endl;
	//delete t;

	//t = new sequence_trie(state2,58,pay); *pay = 10;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->test_and_insert(state,57,pay); *pay = 20;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->test_and_insert(state,55,pay); *pay = 30;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//cout << "PAY " << *pay << endl;
	//delete t;
}



uint64_t hash_state(const vector<uint64_t> & v) {
	size_t r = 0;
	for (const uint64_t & x : v)
		r = r ^ x;
	return r;
}













VisitedList::VisitedList(Model *m) {
    this->htn = m;
    this->useTotalOrderMode = this->htn->isTotallyOrdered;
    this->useSequencesMode = this->htn->isParallelSequences;
    this->canDeleteProcessedNodes = this->useTotalOrderMode || this->useSequencesMode;
#ifdef NOVISI
    this->canDeleteProcessedNodes = true;
#endif
#ifndef POVISI_EXACT
    this->canDeleteProcessedNodes = true;
#endif
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

int VisitedList::getHash(vector<int> *seq) {
    const int bigP = 2147483647;
    long hash = 0;
    for (int i = 0; i < seq->size(); i++) {
        hash *= htn->numTasks;
        hash += seq->at(i);
        hash %= bigP;
    }
    hash += INT32_MIN;
    return (int) hash;
}

bool insertVisi2(searchNode *n) {
    return true;
}

bool VisitedList::insertVisi(searchNode *n) {
#ifdef NOVISI
    return true;
#endif


    //vector<int> *traversal = topSort(n);
    //int hash2 = getHash(traversal);




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

    attemptedInsertions++;
    std::clock_t before = std::clock();
    vector<uint64_t> ss = state2Int(n->state);

#if (TOVISI == TOVISI_PRIM) || (TOVISI == TOVISI_PRIM_EXACT)
    long lhash = 1;
    for (int i = 0; i < n->numContainedTasks; i++) {
        int numTasks = this->htn->numTasks;
        int task = n->containedTasks[i];
        int count = n->containedTaskCount[i];
        //cout << task << " " << count << endl;
        for (int j = 0; j < count; j++) {
            int p_index = j * numTasks + task;
            int p = getPrime(p_index);
            //icout << "p: " << p << endl;
            lhash = lhash * p;
            lhash = lhash % 104729;
        }
    }
    int hash = (int) lhash;
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
}
