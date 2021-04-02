#ifndef PANDAPIENGINE_VISITEDLIST_H
#define PANDAPIENGINE_VISITEDLIST_H

#include "../ProgressionNetwork.h"
#include <unordered_map>



struct hash_table{
	int buckets;
	void** table;
	hash_table(): buckets(1024*1024) {
		table = (void**) calloc(buckets, sizeof(void*));
		for (int i = 0; i < buckets; i++) table[i] = nullptr;
	}
	void* & get(int x){return table[x % buckets];}
};




typedef	tuple<vector<uint64_t>
#ifdef POVISI_HASH	
		,int
#endif
#ifdef POVISI_LAYERS	
		,vector<unordered_map<int,int>>
#endif
#ifdef POVISI_ORDERPAIRS	
		,unordered_set<pair<int,int>>
#endif
	> po_hash_tuple;



using namespace progression;

namespace std {

template<>
struct hash<pair<int,int>> {
	size_t operator()(const pair<int,int> & v) const {
		return v.first + v.second * 1000003;
	}
};


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
struct hash<po_hash_tuple> {
	size_t operator()(const po_hash_tuple & arg) const {
		size_t h = hash<vector<uint64_t>>{}(get<0>(arg));

#define POS1 1
#ifdef POVISI_HASH			
		h = h ^ get<POS1>(arg);
#define POS2 (POS1+1)
#else
#define POS2 POS1
#endif


#ifdef POVISI_LAYERS			
		size_t hhh = 0;
		for (int d = 0; d < get<POS2>(arg).size(); d++){
			size_t hh = 0;
			for (auto & [a,b] : get<POS2>(arg)[d])
				hh = (hh*9973*9973 + a*9973 + b);
			hhh = (hhh*1000003 + hh);
		}
		h = h ^ hhh;
#define POS3 (POS2+1)
#else
#define POS3 POS2
#endif


#ifdef POVISI_ORDERPAIRS			
		size_t hh = 0;
		for (auto & [a,b] : get<POS3>(arg))
			hh = (hh*9973*9973 + a*9973 + b);
		h = h ^ hh;
#define POS4 (POS3+1)
#else
#define POS4 POS3
#endif
		return h;
	}
};
}

struct VisitedList{
	VisitedList(Model * m);
	
	// insert the node into the visited list
	// @returns true if the node was *new* and false, if the node was already contained in the visited list
	bool insertVisi(searchNode * node);

	bool canDeleteProcessedNodes;
	
	int attemptedInsertions = 0;
	int uniqueInsertions = 0;
	int subHashCollision = 0;
	double time = 0;

private:
	Model * htn;
	bool useTotalOrderMode;
	bool useSequencesMode;


	hash_table stateTable;	






#if (TOVISI == TOVISI_SEQ)
	map<vector<uint64_t>, set<vector<int>>> visited;
#elif (TOVISI == TOVISI_PRIM)
	map<vector<uint64_t>, unordered_set<int>> visited;
#elif (TOVISI == TOVISI_PRIM_EXACT)
	map<vector<uint64_t>, map<int,set<vector<int>>>> visited;
#endif

#ifdef POVISI_EXACT
	unordered_map<
#else
	unordered_set<
#endif
	po_hash_tuple
#ifdef POVISI_EXACT
   	, set< vector<vector<int>> >
#endif
	> po_seq_occ;


#ifdef POVISI_EXACT
	unordered_map<
#else
	unordered_set<
#endif
	po_hash_tuple
#ifdef POVISI_EXACT
   	, vector<searchNode*>
#endif
	> po_occ;


    vector<int> *topSort(searchNode *n);

    int getHash(vector<int> *pVector);
};


#endif
