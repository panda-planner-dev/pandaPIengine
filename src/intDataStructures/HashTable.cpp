#include "HashTable.h"
#include <cstdlib>

hash_table::hash_table(): hash_table(1024*1024) {}

hash_table::hash_table(int numBuckets): buckets(numBuckets) {
	table = (void**) calloc(buckets, sizeof(void*));
	for (int i = 0; i < buckets; i++) table[i] = nullptr;
}

void** hash_table::get(size_t x){
	return table + (x % buckets);
}


