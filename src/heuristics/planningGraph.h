/*
 * planninggGraph.h
 *
 *  Created on: 26.10.2018
 *      Author: Daniel Höller
 */

#ifndef HEURISTICS_PLANNINGGRAPH_H_
#define HEURISTICS_PLANNINGGRAPH_H_

#include <set>
#include <climits>
#include "../intDataStructures/IntPairHeap.h"
#include "../intDataStructures/bucketSet.h"
#include "../intDataStructures/noDelIntSet.h"
#include "../intDataStructures/IntStack.h"
#include "../intDataStructures/delIterIntSet.h"
#include "../Model.h"

namespace progression {

class planningGraph {
public:
	planningGraph(Model* sas);
	virtual ~planningGraph();

	void calcReachability(vector<bool>& s, noDelIntSet& reachable);
	bool factReachable(int i);
	bool taskReachable(int i);
	bool methodReachable(int i);

	Model* m;
	noDelIntSet usefulFactSet;
	noDelIntSet reachableTasksSet;
	noDelIntSet reachableMethodsSet;
private:
	IntPairHeap<int>* queue;
	IntStack* stack;
	int* hValPropInit;

	int* numSatPrecs;
	int* hValOp;
	int* hValProp;

	noDelIntSet markedFs;
	noDelIntSet markedOps;
	IntStack needToMark;

	int* subtasks;

};

} /* namespace progression */

#endif /* HEURISTICS_PLANNINGGRAPH_H_ */
