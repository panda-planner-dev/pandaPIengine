#ifndef SEQUENCESET_H
#define SEQUENCESET_H

#include <vector>
#include <cstdint>
#include "../flags.h"

using namespace std;

typedef uint16_t payloadType;

struct sequence_trie {
	uint64_t value; // 8 byte
	uint16_t myBlock;
	uint16_t ignoreBitsFirst;
	uint16_t ignoreBitsLast;
	payloadType payload; // 8 byte
	
	uint16_t zeroBlock;
	uint16_t zeroInside;
	uint16_t oneBlock;
	uint16_t oneInside;

	//sequence_trie * zero; // 8 byte
	//sequence_trie * one; // 8 byte

	sequence_trie(const vector<uint64_t> & sequence, int paddingBits, payloadType * & p);
	void insert(const vector<uint64_t> & sequence, int paddingBits, payloadType* & p);
	
	~sequence_trie();
	void print_tree(int indent);
	void print_node(int indent);

	sequence_trie* getZero();
	sequence_trie* getOne();
private:
	void split_me_at(int bit);
	// constructors only used internally
	sequence_trie();
	sequence_trie(const vector<uint64_t> & sequence, int startingBlock, int startingBit, int paddingBits, payloadType* & p);
	void check_integrity();
};

#endif
