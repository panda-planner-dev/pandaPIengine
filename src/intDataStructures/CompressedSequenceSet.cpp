#include "CompressedSequenceSet.h"
#include "SequenceSetCommon.h"
#include "HashTable.h"
#include <bitset>
#include <iostream>
#include <iomanip>
#include <cassert>
#include "../Debug.h"
// for testing
#include <chrono>
#include <map>
#include <unordered_map>

void compressed_sequence_trie::print_node(int indent){
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

void compressed_sequence_trie::print_tree(int indent){
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

compressed_sequence_trie::compressed_sequence_trie(){
	payload = nullptr;
	blocks = 0;
	ignoreBitsFirst = 0;
	ignoreBitsLast = 0;
	myFirstBlock = 0;
	zero = nullptr;
	one = nullptr;
}

compressed_sequence_trie::~compressed_sequence_trie(){
	free(value);
	delete zero;
	delete one;
}

compressed_sequence_trie::compressed_sequence_trie(const vector<uint64_t> & sequence, int paddingBits, void** & p) : compressed_sequence_trie(){
	blocks = sequence.size();
	p = &payload;
	payload = nullptr;
	myFirstBlock = 0;
	ignoreBitsFirst = 0;
	ignoreBitsLast = paddingBits;
	zero = one = nullptr;
	// copy sequence into value	
	value = (uint64_t*) calloc(sequence.size(), sizeof(uint64_t));
	for (int i = 0; i < sequence.size(); i++)
		value[i] = sequence[i];
}


compressed_sequence_trie::compressed_sequence_trie(const vector<uint64_t> & sequence, int startingBlock, int startingBit, int paddingBits, void** & p) : compressed_sequence_trie(){
	blocks = sequence.size() - startingBlock;
	p = &payload;
	payload = nullptr;
	myFirstBlock = startingBlock;
	ignoreBitsFirst = startingBit;
	ignoreBitsLast = paddingBits;
	zero = one = nullptr;
	// copy sequence into value	
	value = (uint64_t*) calloc(blocks, sizeof(uint64_t));
	for (int i = 0; i < blocks; i++)
		value[i] = sequence[i + startingBlock];

	value[0] = value[0] & bitmask_ignore_first(startingBit);
}

void compressed_sequence_trie::split_me_at(int block, int bit){
	DEBUG(cout << "Starting split of block " << block << " @ bit " << bit << endl);
	
	compressed_sequence_trie * child = new compressed_sequence_trie();
	// child gets all blocks from (inclusive) block to blocks
	child->blocks = blocks - block; DEBUG(cout << "\tchild receives " << child->blocks << " blocks " << endl);
	assert(child->blocks);
	child->myFirstBlock = myFirstBlock + block; DEBUG(cout << "\tfirst block is " << child->myFirstBlock << endl);
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


void compressed_sequence_trie::insert(const vector<uint64_t> & sequence, int paddingBits, void** & p){
	// all of the padding bits must actually be zero, else the testing of the difference breaks
	assert((sequence.back() & ~bitmask_ignore_last(paddingBits)) == 0);

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
						one->insert(sequence, paddingBits, p);
					else
						one = new compressed_sequence_trie(sequence, sequence.size() - 1, 64 - ignoreBitsLast, paddingBits, p);
				} else {
					if (zero)
						zero->insert(sequence, paddingBits, p);
					else
						zero = new compressed_sequence_trie(sequence, sequence.size() - 1, 64 - ignoreBitsLast, paddingBits, p);
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
					one->insert(sequence, paddingBits, p);
				} else {
					DEBUG(cout << "\tCreate." << endl);
					one = new compressed_sequence_trie(sequence, nextBlock, next_bit, paddingBits, p);
				}
			} else {
				DEBUG(cout << "\tNext bit is zero." << endl);
				if (zero) {
					DEBUG(cout << "\tInsert." << endl);
					zero->insert(sequence, paddingBits, p);
				} else {
					DEBUG(cout << "\tCreate." << endl);
					zero = new compressed_sequence_trie(sequence, nextBlock, next_bit, paddingBits, p);
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

	if (sequence[myFirstBlock + block_with_first_difference] & bitmask_bit(splitBit)){
		DEBUG(cout << "New Child One. Block " << myFirstBlock + block_with_first_difference << endl);
		one = new compressed_sequence_trie(sequence,myFirstBlock + block_with_first_difference, splitBit, paddingBits, p);
	} else {
		DEBUG(cout << "New Child Two. Block " << myFirstBlock + block_with_first_difference << endl);
		zero = new compressed_sequence_trie(sequence,myFirstBlock + block_with_first_difference, splitBit, paddingBits, p);
	}

	TEST(check_integrity());
	// split
	return;
}

void compressed_sequence_trie::check_integrity(){
	assert(ignoreBitsLast != 64);
	if (myFirstBlock || ignoreBitsFirst){
		assert(ignoreBitsFirst != 64);
		// must have at least one bit
		if (blocks == 1) assert(ignoreBitsFirst + ignoreBitsLast < 64);
	}

	if (one) one->check_integrity();
	if (zero) zero->check_integrity();
}


void stat(compressed_sequence_trie * t, int & nodes, int & fill){
	int a = t->ignoreBitsFirst;
	int b = t->ignoreBitsLast;
	nodes++;
	fill += 64 - a - b;
	if (t->zero) stat(t->zero, nodes, fill);
	if (t->one ) stat(t->one , nodes, fill);
}


// only for testing purposes, thus non clean imports ...
void printMemory();

namespace std {

template<>
struct hash<vector<uint64_t>> {
	size_t operator()(const vector<uint64_t> & v) const {
		size_t r = 0;
		for (const uint64_t & x : v)
			r = r ^ x;
		return r;
	}
};

template<>
struct hash<pair<vector<uint64_t>,int>> {
	size_t operator()(const pair<vector<uint64_t>,int> & v) const {
		return hash<vector<uint64_t>>{}(v.first) ^ v.second;
	}
};

}



void speed_test(){
	cout << "SIZE " << sizeof(compressed_sequence_trie) << endl;

	int numbers = 3 * 1000 * 1000;
	int lenMax = 40;

	setDebugMode(false);

	srand(42);
	std::clock_t beforeMap = std::clock();
	unordered_map<pair<vector<uint64_t>,int>,int> data;
	//map<pair<vector<uint64_t>,int>,int> data;
	for (int j = 0 ; j < numbers; j++){
		vector<uint64_t> vec;
		//int len = rand() % lenMax;
		int len = lenMax;
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
	hash_table tab (10 * 1000 * 1000);
	printMemory();
	std::clock_t beforeTrie = std::clock();
	for (int j = 0 ; j < numbers; j++){
		vector<uint64_t> vec;
		//int len = rand() % lenMax;
		int len = lenMax;
		for (int i = 0; i < len; i++)
			vec.push_back(uint64_t(rand()) * uint64_t(rand()));
		int padding = rand() % 64;
		vec.push_back((uint64_t(rand()) * uint64_t(rand())) & bitmask_ignore_last(padding));

		size_t h = hash<vector<uint64_t>>{}(vec);
		compressed_sequence_trie ** t = (compressed_sequence_trie**) tab.get(h);

		void* *pay;
		if (!*t)
			*t = new compressed_sequence_trie(vec,padding,pay);
		else {
			(*t)->insert(vec,padding,pay);
		}
		
		*(uint64_t*)pay = j+1;
	}
	
	std::clock_t afterTrie = std::clock();
	double trie_time = 1000.0 * (afterTrie - beforeTrie) / CLOCKS_PER_SEC;
	cout << "Trie took: " << setprecision(3) << fixed << trie_time << " ms" << endl;
	printMemory();

	//int n = 0, f = 0;
	//stat(t,n,f);
	//cout << "N " << n << " F " << f << " " << (double(f) / n) << endl;
}

void test(){
	setDebugMode(false);
	srand(42);
	compressed_sequence_trie * t = nullptr; 

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

		void* *pay;
		if (!t)
			t = new compressed_sequence_trie(vec,padding,pay);
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
			assert(*(uint64_t*)pay == data[make_pair(vec,padding)]);
	
		*(uint64_t*)pay = j + 1;
		data[make_pair(vec,padding)] = *(uint64_t*)pay;

		// check integrity of the tree in every step
		for (auto [key,value] : data){
			void* *pay = nullptr;
			auto [vec,padding] = key;
			t->insert(vec,padding,pay);
			DEBUG(cout << "checking "; for (uint64_t v : vec) cout << v << ","; cout << " pad: " << padding << " @ " << pay << endl);
			assert(*(uint64_t*)pay == value);
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
	//compressed_sequence_trie * t = new compressed_sequence_trie(state2,55,pay); *pay = 10;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->insert(state3,55,pay); *pay = 20;
	//cout << endl;
	//t->print_tree(0);

	//int* pay;
	//compressed_sequence_trie * t = new compressed_sequence_trie(state,55,pay); *pay = 10;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->insert(state,57,pay); *pay = 20;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->insert(state2,58,pay); *pay = 30;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//cout << "PAY " << *pay << endl;
	//delete t;

	//t = new compressed_sequence_trie(state2,58,pay); *pay = 10;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->insert(state,57,pay); *pay = 20;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//t->insert(state,55,pay); *pay = 30;
	//cout << endl;
	//t->print_tree(0);
	//cout << endl;
	//cout << "PAY " << *pay << endl;
	//delete t;
}


