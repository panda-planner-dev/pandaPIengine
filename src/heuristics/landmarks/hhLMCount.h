/*
 * hhLMCount.h
 *
 *  Created on: 08.01.2020
 *      Author: dh
 */

#ifndef HEURISTICS_HHLMCOUNT_H_
#define HEURISTICS_HHLMCOUNT_H_

#include "../../Model.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "../../intDataStructures/IntUtil.h"
#include "../planningGraph.h"

namespace progression {
//#if ((HEURISTIC == LMCANDOR) || (HEURISTIC == LMCLOCAL))

class hhLMCount {
public:
	hhLMCount(Model* htn, searchNode *n, int typeOfLMs);
	virtual ~hhLMCount();
	IntUtil iu;

	const int localLMs = 0;
	const int andOrLMs = 1;
	const int fdLMs = 2;


	const int fTask = 0; // state feature of new model that belonged to task in original HTN
	const int fFact = 1; // state feature of new model that belonged to state feature in original HTN

	const Model* m;

	void setHeuristicValue(searchNode *n, searchNode *parent, int action);
	void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method);

	void prettyPrintLMs(Model* htn, searchNode *n);

	lookUpTab* createElemToLmMapping(searchNode *tnI, lmType type);
	void deleteFulfilledLMs(searchNode *tnI);
private:
	void setHeuristicValue(searchNode *n);

#ifdef LMCANDORRA
	planningGraph* pg = nullptr;
	noDelIntSet preProReachable;
	int pruned = 0;
#endif
};
//#endif
} /* namespace progression */
#endif /* HEURISTICS_HHLMCOUNT_H_ */
