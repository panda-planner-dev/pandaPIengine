#ifndef TRANSCONTROLLER_H_
#define TRANSCONTROLLER_H_

#include "../Model.h"


enum TranslationType{
	ParallelSeq, Push
};


void runTranslationPlanner(Model* htn, TranslationType transtype, int pgb, string downward, string sasfile,
		bool iterate,
		bool onlyGenerate);

#endif
