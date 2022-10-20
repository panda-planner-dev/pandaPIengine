/*
 * RCModelFactory.h
 *
 *  Created on: 09.02.2020
 *      Author: dh
 */

#ifndef HEURISTICS_RCHEURISTICS_RCMODELFACTORY_H_
#define HEURISTICS_RCHEURISTICS_RCMODELFACTORY_H_

#include <cassert>
#include "../../Model.h"
#include "../../intDataStructures/StringUtil.h"

namespace progression {

class RCModelFactory {
public:
	RCModelFactory(Model* htn);
	virtual ~RCModelFactory();
	Model* getRCmodelSTRIPS();
	Model* getRCmodelSTRIPS(int costsMethodActions, int costRegularActions);
	void createInverseMappings(Model* c);
	int t2tdr(int task);
	int t2bur(int task);

	pair<int, int> rcStateFeature2HtnIndex(string s);

	int fTask = 0; // state feature of new model that belonged to task in original HTN
	int fFact = 1; // state feature of new model that belonged to state feature in original HTN
	Model* htn;

private:
	StringUtil su;
};

} /* namespace progression */

#endif /* HEURISTICS_RCHEURISTICS_RCMODELFACTORY_H_ */
