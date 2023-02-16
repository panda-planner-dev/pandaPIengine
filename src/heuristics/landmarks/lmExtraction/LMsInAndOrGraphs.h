/*
 * LMsInAndOrGraphs.h
 *
 *  Created on: 30.11.2019
 *      Author: dh
 */

#ifndef LMSINANDORGRAPHS_H_
#define LMSINANDORGRAPHS_H_

#include <set>
#include "../../../Model.h"
#include "../../../intDataStructures/IntPairHeap.h"

namespace progression {

class LMsInAndOrGraphs {
public:
	LMsInAndOrGraphs(Model* htn);
	virtual ~LMsInAndOrGraphs();
	void generateAndOrLMs(searchNode* tnI);
	void generateLocalLMs(Model* htn, searchNode* tnI);
	set<int>* genLocalLMs(Model* htn, int task);

	set<int>* flm;
	set<int>* mlm;
	set<int>* tlm;

	const int tAND = 0;
	const int tOR = 1;
	const int tINIT = 2;

	const bool silent = false;

	int getNumLMs();
	landmark** getLMs();

	void prettyPrintGraph();

private:
	Model* htn = nullptr;
	int fNode(int i);
	int aNode(int i);
	int mNode(int i);
	int nodeToF(int i);
	int nodeToA(int i);
	int nodeToM(int i);
	bool isFNode(int i);
	bool isANode(int i);
	bool isMNode(int i);

	bool* fullSet = nullptr;
	int numNodes = -1;
	int* nodeType = nullptr;
	vector<int>* Ninv = nullptr;
	vector<int>* N = nullptr;
	set<int>* LMs = nullptr;
	IntPairHeap<int>* heap = nullptr;
	set<int>* temp = nullptr;
	set<int>* temp2 = nullptr;
	int* tasksInTNI = nullptr;

};

} /* namespace progression */

#endif /* LMSINANDORGRAPHS_H_ */
