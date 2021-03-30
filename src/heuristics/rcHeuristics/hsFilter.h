/*
 * hsFilter.h
 *
 *  Created on: 19.12.2018
 *      Author: dh
 */

#ifndef HEURISTICS_HSFILTER_H_
#define HEURISTICS_HSFILTER_H_

#include "../../intDataStructures/IntPairHeap.h"
#include "../../intDataStructures/bucketSet.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "../../intDataStructures/IntStack.h"
#include "hsAddFF.h"
#include "LMCutLandmark.h"

namespace progression {

class hsFilter {
public:
	hsFilter(Model* sas);
	virtual ~hsFilter();
		
	string getDescription(){ return "filter";}

    list<LMCutLandmark *>* cuts = new list<LMCutLandmark *>();
	
	int getHeuristicValue(bucketSet& s, noDelIntSet& g);
	Model* m;
private:
	hsAddFF* add;
};
} /* namespace progression */

#endif /* HEURISTICS_HSFILTER_H_ */
