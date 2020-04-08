#ifndef pdt_h_INCLUDED
#define pdt_h_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"
#include "sog.h"


struct PDT {
	vector<int> possiblePrimitives;
	vector<int> possibleAbstracts;

	bool expanded;

	int numberOfChildren;
	vector<PDT*> children;
	SOG* sog;

	vector<vector<bool>> applicableMethods; // indicates whether the methods in the model are actually applicable and have not been pruned
	vector<vector<vector<int>>> positionOfChildrenForMethods;
	vector<int> positionOfPrimitivesInChildren;
};

PDT* initialPDT(Model* htn);
void expandPDT(PDT* pdt, Model* htn);
void expandPDTUpToLevel(PDT* pdt, int K, Model* htn);

#endif
