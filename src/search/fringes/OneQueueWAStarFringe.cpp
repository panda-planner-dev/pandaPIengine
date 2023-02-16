//
// Created by dh on 29.03.21.
//

#include "OneQueueWAStarFringe.h"



////////////////////////////////
// CmpNodePtrs
////////////////////////////////

bool TieBreakingNodePointerComaprator::operator()(const searchNode* a, const searchNode* b) const {
	if (a->fValue != b->fValue)
		return a->fValue > b->fValue;
	else {
		for (int i = 1; i < numOfHeuristics; i++)
			if (a->heuristicValue[i] != b->heuristicValue[i])
				return a->heuristicValue[i] > b->heuristicValue[i];
		// if everything fails, use the fixed random numbers per search node
		return a->hRand > b->hRand;
	}
}

bool OneQueueWAStarFringe::isEmpty() {
    return fringe.empty();
}

searchNode* OneQueueWAStarFringe::pop() {
    searchNode* top = fringe.top();
	fringe.pop();
	return top;
}

void OneQueueWAStarFringe::push(searchNode *n) {
	n->hRand = rand();
	// compute the f values for this search node
	n->fValue = n->heuristicValue[0] * hWeight;

	switch (aStarOption){
		case gValNone:  /* nothing to do */ break;
		case gValPathCosts: n->fValue += n->modificationDepth; break;
		case gValActionCosts: n->fValue += n->actionCosts; break;
		case gValActionPathCosts: n->fValue += n->mixedModificationDepth; break;
	}
	
	fringe.push(n);
}


void OneQueueWAStarFringe::printTypeInfo(){
	if (aStarOption == gValNone){
		cout << "- Greedy Search" << endl;
	} else {
		if (hWeight == 1)
			cout << "- A* Search" << endl;
		else
			cout << "- Greedy A* Search with weight " << hWeight << endl;
		
		switch (aStarOption){
			case gValPathCosts:
				cout << "- Distance G is \"modification depth\"" << endl; break;
			case gValActionCosts:
				cout << "- Distance G is \"action costs\"" << endl; break;
			case gValActionPathCosts:
				cout << "- Distance G is \"action costs\" + \"number of decompositions\"" << endl; break;
			case gValNone:  /* cannot happen */ break;
		}
	}
}

int OneQueueWAStarFringe::size() {
    return fringe.size();
}
