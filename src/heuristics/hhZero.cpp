/*
 * HtnhZero.cpp
 *
 *  Created on: 16.09.2017
 *      Author: Daniel Höller
 */

#include "hhZero.h"

namespace progression {

hhZero::hhZero(Model* htn, int index) : Heuristic(htn, index){
	// TODO Auto-generated constructor stub

}

hhZero::~hhZero() {
	// TODO Auto-generated destructor stub
}

void hhZero::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
	n->heuristicValue[index] = 0;
	n->goalReachable = true;
}
void hhZero::setHeuristicValue(searchNode *n, searchNode *parent, int absTask,
		int method) {
	n->heuristicValue[index] = 0;
	n->goalReachable = true;
}

} /* namespace progression */
