#ifndef COMPRESSEDSEQUENCESET_H
#define COMPRESSEDSEQUENCESET_H

#include <vector>
#include <cstdint>
#include "../flags.h"

using namespace std;

struct compressed_sequence_trie {
	void* payload;
	uint64_t * value;
	uint16_t blocks;
	uint16_t myFirstBlock;
	uint16_t ignoreBitsFirst;
	uint16_t ignoreBitsLast;
	
	compressed_sequence_trie * zero;
	compressed_sequence_trie * one;

	compressed_sequence_trie(const vector<uint64_t> & sequence, int paddingBits, void* * & p);
	void insert(const vector<uint64_t> & sequence, int paddingBits, void** & p);
	
	~compressed_sequence_trie();
	void print_tree(int indent);
	void print_node(int indent);
private:
	void split_me_at(int block, int bit);
	// constructors only used internally
	compressed_sequence_trie();
	compressed_sequence_trie(const vector<uint64_t> & sequence, int startingBlock, int startingBit, int paddingBits, void** & p);
	void check_integrity();
};

#endif
