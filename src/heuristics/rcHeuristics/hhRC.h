/*
 * Heuristic.h
 *
 *  Created on: 22.09.2017
 *      Author: Daniel Höller
 */

#ifndef HEURISTICS_HHRC_H_
#define HEURISTICS_HEURISTIC_H_

#include <set>
#include <forward_list>
#include "../../Model.h"
#include "../../intDataStructures/bucketSet.h"
#include "../../intDataStructures/bIntSet.h"
#include "../../intDataStructures/noDelIntSet.h"
#include "hsAddFF.h"
#include "hsLmCut.h"
#include "hsFilter.h"

namespace progression {

class hhRC {
private:
	/*set<int> s0set;
	 set<int> gset;
	 set<int> intSet;*/
	noDelIntSet gset;
	noDelIntSet intSet;
#if STATEREP == SRCALC1
	// todo: then parallelized, this var must be per core:
	bucketSet s0set;
	void collectState(solutionStep* sol, bucketSet& s0set);
#elif STATEREP == SRCALC2
	// todo: then parallelized, this var must be per core:
	noDelIntSet s0set;
	void collectState(solutionStep* sol, noDelIntSet& adds, noDelIntSet& dels);
#elif STATEREP == SRCOPY
	bucketSet s0set;
#elif STATEREP == SRLIST
	noDelIntSet s0set;
#endif
	void calcHtnGoalFacts(planStep *ps);
	int t2tdr(int task);
	int t2bur(int task);
	void setHeuristicValue(searchNode *n);
public:
#if (HEURISTIC == RCFF || HEURISTIC == RCADD)
	hsAddFF* sasH;
	hhRC(Model* htn, hsAddFF* sasH);
#elif (HEURISTIC == RCLMC)
	hsLmCut* sasH;
	hhRC(Model* htn, hsLmCut* sasH);
#else
	hsFilter* sasH;
	hhRC(Model* htn, hsFilter* sasH);
#endif
	const Model* m;
	virtual ~hhRC();
	void setHeuristicValue(searchNode *n, searchNode *parent, int action);
	void setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method);
};

} /* namespace progression */

#endif /* HEURISTICS_HHRC_H_ */
