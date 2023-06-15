#ifndef PANDAPIENGINE_VISITEDLIST_H
#define PANDAPIENGINE_VISITEDLIST_H

#include <stdint.h>
#include "../ProgressionNetwork.h"
#include <unordered_map>
#include "../intDataStructures/HashTable.h"


// XXX magic number, restricts number of copies of a task per task net to 255, thereafter the hash becomes incorrect
// 3 -> up to three bits for (number-1), max is 8, thus whole number is max 225
const int number_of_bits_for_task_count = 3;
const int max_task_count = (1 << (1 << number_of_bits_for_task_count)) - 1;

using namespace progression;

struct VisitedList{
	VisitedList(Model *m, bool _noVisitedCheck, bool _noReOpening, bool _taskHash, bool _taskSequenceHash, bool _topologicalOrdering, bool _orderPairs, bool _layers, bool _allowGIcheck, bool _allowedToUseParallelSequences);
	
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
	// automatically determined configuration
	bool useTotalOrderMode;
	bool useSequencesMode;
	int bitsNeededPerTask;
	// configuration given by the user
	bool noVisitedCheck;
	bool noReopening;
	bool GIcheck;
	bool taskHash;
	bool sequenceHash;
	bool topologicalOrdering;
	bool orderPairs;
	bool layers;

	uint64_t hashingP;

	hash_table * stateTable;

	uint64_t taskCountHash(searchNode * n);
	uint64_t taskSequenceHash(vector<int> & tasks);

    vector<int> topSort(searchNode *n);

    uint32_t getHash(vector<int> *pVector);
};


#endif
