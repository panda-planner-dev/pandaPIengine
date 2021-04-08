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
#include "../Heuristic.h"

namespace progression {

enum lmFactory {lmfLOCAL, lmfANDOR, lmfFD};

class hhLMCount : public Heuristic {
public:
	hhLMCount(Model* htn, int index, searchNode *n, lmFactory typeOfFactory);
	virtual ~hhLMCount();
	IntUtil iu;

	const int fTask = 0; // state feature of new model that belonged to task in original HTN
	const int fFact = 1; // state feature of new model that belonged to state feature in original HTN

	const Model* m;

	void setHeuristicValue(searchNode *n, searchNode *parent, int action) override;
	void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) override;
    string getDescription() override;

//	void prettyPrintLMs(Model* htn, searchNode *n);
//
//	lookUpTab* createElemToLmMapping(searchNode *tnI, lmType type);
//	void deleteFulfilledLMs(searchNode *tnI);
//private:
//	void setHeuristicValue(searchNode *n);

#ifdef LMCANDORRA
	planningGraph* pg = nullptr;
	noDelIntSet preProReachable;
	int pruned = 0;
#endif
};
} /* namespace progression */
#endif /* HEURISTICS_HHLMCOUNT_H_ */
