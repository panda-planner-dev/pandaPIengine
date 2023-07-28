/*
 * hhSimple.h
 *
 *  Created on: 29.01.2023
 *      Author: Gregor Behnke
 */

#include "hhSimple.h"

namespace progression {

hhModDepth::hhModDepth(Model* htn, int index, bool _invert) : Heuristic(htn, index){invert = _invert;}
hhModDepth::~hhModDepth() {}

void hhModDepth::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
	n->heuristicValue[index] = n->modificationDepth * (invert?-1:1);
	n->goalReachable = true;
}
void hhModDepth::setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) {
	n->heuristicValue[index] = n->modificationDepth * (invert?-1:1);
	n->goalReachable = true;
}

hhMixedModDepth::hhMixedModDepth(Model* htn, int index, bool _invert) : Heuristic(htn, index){invert = _invert;}
hhMixedModDepth::~hhMixedModDepth() {}

void hhMixedModDepth::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
	n->heuristicValue[index] = n->mixedModificationDepth * (invert?-1:1);
	n->goalReachable = true;
}
void hhMixedModDepth::setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) {
	n->heuristicValue[index] = n->mixedModificationDepth * (invert?-1:1);
	n->goalReachable = true;
}

hhCost::hhCost(Model* htn, int index, bool _invert) : Heuristic(htn, index){invert = _invert;}
hhCost::~hhCost() {}

void hhCost::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
	n->heuristicValue[index] = n->actionCosts * (invert?-1:1);
	n->goalReachable = true;
}
void hhCost::setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) {
	n->heuristicValue[index] = n->actionCosts * (invert?-1:1);
	n->goalReachable = true;
}

} /* namespace progression */
