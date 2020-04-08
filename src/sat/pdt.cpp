#include "pdt.h"

PDT* initialPDT(Model* htn){
	PDT* pdt = new PDT;
	pdt->expanded = false;
	pdt->possibleAbstracts.push_back(htn->initialTask);

	return pdt;
}


void expandPDT(PDT* pdt, Model* htn){
	if (pdt->expanded) return;
	if (pdt->possibleAbstracts.size() == true) return;

	vector<int> applicableMethodsForSOG;
	// gather applicable methods
	for (int tIndex = 0; tIndex < pdt->possibleAbstracts.size(); tIndex++){
		int & t = pdt->possibleAbstracts[tIndex];
		for (int m = 0; m < htn->numMethodsForTask[t]; m++){
			pdt->applicableMethods[tIndex].push_back(true);
			applicableMethodsForSOG.push_back(htn->taskToMethods[t][m]);
		}
	}

	// get best possible SOG
	optimiseSOG(applicableMethodsForSOG, htn);
}


void expandPDTUpToLevel(PDT* pdt, int K, Model* htn){

}

