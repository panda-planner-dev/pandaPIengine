#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <cstddef>

struct hash_table{
	int buckets;
	void** table;
	hash_table();
	hash_table(int numBuckets);
	// needs to be unsigned!
	void** get(size_t x);
};

#endif
