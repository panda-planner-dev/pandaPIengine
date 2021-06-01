#include "SequenceSet.h"
#include "SequenceSetCommon.h"
#include <bitset>
#include <iostream>
#include <iomanip>
#include <cassert>
#include "../Debug.h"
// for testing
#include <chrono>
#include <map>


const int cellsPerBlock = 65536;
int nextCell = cellsPerBlock - 1;

vector<sequence_trie*> memory_blocks;


sequence_trie* alloc(uint16_t & block, uint16_t & inside){
	if (nextCell == cellsPerBlock - 1){
		nextCell = 0;
		memory_blocks.push_back((sequence_trie*) malloc(sizeof(sequence_trie) * cellsPerBlock));
		if (memory_blocks.size() == 1) nextCell++; // 0 should not be a valid address ...
	}

	sequence_trie* ret = memory_blocks.back() + nextCell;
	inside = nextCell;
	block = memory_blocks.size() - 1;
	nextCell++;

	return ret;
}

inline sequence_trie* getCell(uint16_t block, uint16_t inside){
	return memory_blocks[block] + inside;
}

sequence_trie* sequence_trie::getZero(){
	return getCell(zeroBlock,zeroInside);
}

sequence_trie* sequence_trie::getOne(){
	return getCell(oneBlock,oneInside);
}



void sequence_trie::print_node(int indent){
	for(int i = 0; i < indent; i++) cout << " ";
	cout << "block position " << myBlock << "; ignore first " << int(ignoreBitsFirst) << "; ignore last " << int(ignoreBitsLast) << "; payload " << payload << endl;

	for(int i = 0; i < indent; i++) cout << " ";
	cout << "M " << bitset<64>(bitmask_ignore_first(ignoreBitsFirst)) << endl;
	for(int i = 0; i < indent; i++) cout << " ";
	cout << "V " << bitset<64>(value) << endl;
	for(int i = 0; i < indent; i++) cout << " ";
	cout << "M " << bitset<64>(bitmask_ignore_last(ignoreBitsLast)) << endl;
}

void sequence_trie::print_tree(int indent){
	print_node(indent);
	if (oneBlock || oneInside){
		for(int i = 0; i < indent; i++) cout << " ";
		cout << "child one";
		getOne()->print_tree(indent+2);
	}
	if (zeroBlock || zeroInside){
		for(int i = 0; i < indent; i++) cout << " ";
		cout << "child zero";
		getZero()->print_tree(indent+2);
	}
}

sequence_trie::sequence_trie(){
	payload = 0;
	value = 0;
	ignoreBitsFirst = 0;
	ignoreBitsLast = 0;
	myBlock = 0;
	zeroBlock = 0;
	zeroInside = 0;
	oneBlock = 0;
	oneInside = 0;
}

sequence_trie::~sequence_trie(){
	//delete zero;
	//delete one;
}

sequence_trie::sequence_trie(const vector<uint64_t> & sequence, int paddingBits, payloadType* & p) :
	sequence_trie(sequence, 0, 0, paddingBits, p){}

sequence_trie::sequence_trie(const vector<uint64_t> & sequence, int startingBlock, int startingBit, int paddingBits, payloadType* & p) : sequence_trie(){
	assert((sequence.back() & ~bitmask_ignore_last(paddingBits)) == 0);
	p = &payload;
	myBlock = startingBlock;
	ignoreBitsFirst = startingBit;
	value = sequence[myBlock] & bitmask_ignore_first(startingBit);
	if (sequence.size() - 1 == myBlock){
		ignoreBitsLast = paddingBits;
	} else {
		if (sequence[myBlock+1] & bitmask_bit(0)){
			sequence_trie * one = alloc(oneBlock,oneInside);
			new (one) sequence_trie(sequence,startingBlock+1,0,paddingBits,p);
		} else {
			sequence_trie * zero = alloc(zeroBlock,zeroInside);
			new (zero) sequence_trie(sequence,startingBlock+1,0,paddingBits,p);
		}
	}
	TEST(check_integrity());
}

void sequence_trie::split_me_at(int bit){
	DEBUG(cout << "Starting split @ bit " << bit << endl);
	
	uint16_t block, inside;
	sequence_trie * child = alloc(block,inside);
   	new (child) sequence_trie();
	// child gets all blocks from (inclusive) block to blocks
	child->myBlock = myBlock;
	// remove the bits before bit from the child's first block
	child->value = value & bitmask_ignore_first(bit); // copy value to child
	// child inherits my current last padding
	child->ignoreBitsLast = ignoreBitsLast;
	// child ignores everything up to this bit
	child->ignoreBitsFirst = bit;
	
	child->payload = payload;
	payload = 0; // reset my own payload to zero
	child->zeroBlock = zeroBlock;
	child->zeroInside = zeroInside;
	child->oneBlock =  oneBlock;
	child->oneInside = oneInside;
	
	// the child is now my child
	zeroBlock = 0;
	zeroInside = 0;
	oneBlock = 0;
	oneInside = 0;
	if (child->value & bitmask_bit(bit)){
		oneBlock = block;
		oneInside = inside;
	} else {
		zeroBlock = block;
		zeroInside = inside;
	}

	// curb myself
	ignoreBitsLast = 64 - bit;
	if (ignoreBitsLast != 64){
		value = value & bitmask_ignore_last(ignoreBitsLast);
	} else {
		// I might end up with no bits at all (edge cases ...)
	}
	
	//print_tree(0);
	TEST(check_integrity());
}


void sequence_trie::insert(const vector<uint64_t> & sequence, int paddingBits, payloadType* & p){
	// all of the padding bits must actually be zero, else the testing of the difference breaks
	assert((sequence.back() & ~bitmask_ignore_last(paddingBits)) == 0);

	DEBUG(cout << endl << endl << "Calling test and insert; my block " << myBlock << endl; print_node(2));
	bool lastBlockOfInput = (myBlock == sequence.size() - 1);


	uint64_t input_test = sequence[myBlock];
	DEBUG(cout << "testing  block: " << bitset<64>(input_test) << endl);
	input_test = input_test & bitmask_ignore_first(ignoreBitsFirst);
	input_test = input_test & bitmask_ignore_last(ignoreBitsLast);

	uint64_t value_test = value;
	// if the input sequence is shorter then my value, I can only compare up to this length
	if (lastBlockOfInput) value_test = value_test & bitmask_ignore_last(paddingBits);
	
	DEBUG(cout << "testing  block: " << bitset<64>(input_test) << endl);
	DEBUG(cout << "sequence block: " << bitset<64>(value_test) << endl);
	
	// no difference found
	if (input_test == value_test){
		// check if the end of the overall sequence is the end of this value
		if (lastBlockOfInput){
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
					if (oneBlock || oneInside)
						getOne()->insert(sequence, paddingBits, p);
					else{
						sequence_trie * one = alloc(oneBlock,oneInside);
						new (one) sequence_trie(sequence, myBlock, 64 - ignoreBitsLast, paddingBits, p);
					}
				} else {
					if (zeroBlock || zeroInside)
						getZero()->insert(sequence, paddingBits, p);
					else {
						sequence_trie * zero = alloc(zeroBlock,zeroInside);
						new (zero) sequence_trie(sequence, myBlock, 64 - ignoreBitsLast, paddingBits, p);
					}
				}
				TEST(check_integrity());
				return;
			} else {
				DEBUG(cout << "\tNew sequence ends within." << endl);
				assert(ignoreBitsLast < paddingBits); // need to split
				int splitBit = 64 - paddingBits;
				// split
				split_me_at(splitBit);
				p = &payload;
				TEST(check_integrity());
				return;
			}
		} else if (myBlock < sequence.size() - 1) {
			DEBUG(cout << "\tNew sequence ends beyond." << endl);
			// this segment is fully equal and there is a next block
			// continue with child
			int next_bit = 64 - ignoreBitsLast;
			if (next_bit == 64) next_bit = 0;
			int nextBlock = myBlock + ((ignoreBitsLast)?0:1);
			DEBUG(cout << "\tBlock " << nextBlock << " Bit " << next_bit << endl);
			uint64_t next_bit_value = sequence[nextBlock] & bitmask_bit(next_bit);

			if (next_bit_value){
				DEBUG(cout << "\tNext bit is one." << endl);
				if (oneBlock || oneInside){
					DEBUG(cout << "\tInsert." << endl);
					getOne()->insert(sequence, paddingBits, p);
				} else {
					DEBUG(cout << "\tCreate." << endl);
					sequence_trie * one = alloc(oneBlock,oneInside);
					new (one) sequence_trie(sequence, nextBlock, next_bit, paddingBits, p);
				}
			} else {
				DEBUG(cout << "\tNext bit is zero." << endl);
				if (zeroBlock || zeroInside) {
					DEBUG(cout << "\tInsert." << endl);
					getZero()->insert(sequence, paddingBits, p);
				} else {
					DEBUG(cout << "\tCreate." << endl);
					sequence_trie * zero = alloc(zeroBlock,zeroInside);
					new (zero) sequence_trie(sequence, nextBlock, next_bit, paddingBits, p);
				}
			}
			TEST(check_integrity());
			return;
		} else {
			// this segment captures more than the segment, but is fully equal
			// --> just split
			DEBUG(cout << "\tNew sequence ends within." << endl);
			int splitBit = 64 - paddingBits;
			// split
			split_me_at(splitBit);
			p = &payload;
			TEST(check_integrity());
			return;
		}
	}

	int splitBit = -1;
	// find first position with difference in this block
	for (int i = 0; i < 64; i++)
		if ((input_test & bitmask_bit(i)) != (value & bitmask_bit(i))){
			splitBit = i;
			break;
		}
	
	assert(splitBit != -1);
	DEBUG(cout << "Split Bit  : " << splitBit << endl);

	// split me	
	split_me_at(splitBit);

	if (sequence[myBlock] & bitmask_bit(splitBit)){
		sequence_trie * one = alloc(oneBlock,oneInside);
		new (one) sequence_trie(sequence, myBlock, splitBit, paddingBits, p);
	} else {
		sequence_trie * zero = alloc(zeroBlock,zeroInside);
		new (zero) sequence_trie(sequence, myBlock, splitBit, paddingBits, p);
	}

	TEST(check_integrity());
	// split
	return;
}

void sequence_trie::check_integrity(){
	if (myBlock || ignoreBitsFirst){
		assert(ignoreBitsLast != 64);
		assert(ignoreBitsFirst != 64);
		// must have at least one bit
		assert(ignoreBitsFirst + ignoreBitsLast < 64);
	}

	assert((value & ~bitmask_ignore_first(ignoreBitsFirst)) == 0);
	assert((value & ~bitmask_ignore_last(ignoreBitsLast)) == 0);

	if (oneBlock || oneInside) getOne()->check_integrity();
	if (zeroBlock || zeroInside) getZero()->check_integrity();
}


void stat(sequence_trie * t, int & nodes, int & fill){
	int a = t->ignoreBitsFirst;
	int b = t->ignoreBitsLast;
	nodes++;
	fill += 64 - a - b;
	if (t->zeroBlock || t->zeroInside) stat(t->getZero(), nodes, fill);
	if (t->oneBlock  || t->oneInside ) stat(t->getOne() , nodes, fill);
}


// only for testing purposes, thus non clean imports ...
void printMemory();


void sequence_trie_speed_test(){
	cout << "SIZE " << sizeof(sequence_trie) << endl;
	cout << "SIZE " << sizeof(pair<vector<uint64_t>,int>) << endl;

	int numbers = 5 * 1000 * 1000;
	int lenMax = 40;

	setDebugMode(false);

	srand(42);
	printMemory();
	std::clock_t beforeMap = std::clock();
	map<pair<vector<uint64_t>,int>,int> data;
	int ds = 0;
	for (int j = 0 ; j < numbers; j++){
		vector<uint64_t> vec;
		int len = rand() % lenMax;
		ds += len*8;
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
	cout << "\tsize of data " << ds << endl;
	srand(42);

	printMemory();
	sequence_trie * t = nullptr; 
	ds = 0;
	std::clock_t beforeTrie = std::clock();
	for (int j = 0 ; j < numbers; j++){
		vector<uint64_t> vec;
		int len = rand() % lenMax;
		ds += len*8;
		for (int i = 0; i < len; i++)
			vec.push_back(uint64_t(rand()) * uint64_t(rand()));
		int padding = rand() % 64;
		vec.push_back((uint64_t(rand()) * uint64_t(rand())) & bitmask_ignore_last(padding));

		//cout << "Do " << j << "/" << numbers << endl;
		payloadType *pay;
		if (!t)
			t = new sequence_trie(vec,padding,pay);
		else {
			t->insert(vec,padding,pay);
		}
		
		*pay = (j+1) % 60000;
	}
	
	std::clock_t afterTrie = std::clock();
	double trie_time = 1000.0 * (afterTrie - beforeTrie) / CLOCKS_PER_SEC;
	cout << "Trie took: " << setprecision(3) << fixed << trie_time << " ms" << endl;
	cout << "\tsize of data " << ds << endl;
	srand(42);
	printMemory();

	int n = 0, f = 0;
	stat(t,n,f);
	cout << "N " << n << " F " << f << " " << (double(f) / n) << endl;
	delete t;
}

void sequence_trie_test(){
	setDebugMode(false);

	srand(42);
	sequence_trie * t = nullptr; 

	map<pair<vector<uint64_t>,int>,int> data;

	for (int j = 0 ; j < 1000; j++){
		vector<uint64_t> vec;
		int len = rand() % 100;
		for (int i = 0; i < len; i++)
			vec.push_back(uint64_t(rand()) * uint64_t(rand()));
		int padding = rand() % 64;
		vec.push_back((uint64_t(rand()) * uint64_t(rand())) & bitmask_ignore_last(padding));
		
		DEBUG(cout << endl << endl << endl);
		cout << "pushing #" << j;
		DEBUG(cout << " "; for (uint64_t v : vec) cout << v << ","; cout << " pad: " << padding;);

		payloadType *pay;
		if (!t)
			t = new sequence_trie(vec,padding,pay);
		else {
			DEBUG(t->print_tree(0));
			t->insert(vec,padding,pay);
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
			payloadType *pay = nullptr;
			auto [vec,padding] = key;
			t->insert(vec,padding,pay);
			DEBUG(cout << "checking "; for (uint64_t v : vec) cout << v << ","; cout << " pad: " << padding << " @ " << pay << endl);
			assert(*pay == value);
		}
	}


	//t->print_tree(0);




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



