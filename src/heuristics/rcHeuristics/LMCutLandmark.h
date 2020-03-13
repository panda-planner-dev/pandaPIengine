//
// Created by dh on 12.03.20.
//

#ifndef PANDAPIENGINE_LMCUTLANDMARK_H
#define PANDAPIENGINE_LMCUTLANDMARK_H

#include "../landmarks/lmDataStructures/landmark.h"

namespace progression {
    class LMCutLandmark {

    public:
        const lmConType contype = disjunctive;
        const lmType lmtype = LMCUT;

        int size = 0;
        int firstMethod = -1;
        int* lm = nullptr;

        bool isMethod(int i);
        bool isAction(int i);
        LMCutLandmark(int size);
    };
}

#endif //PANDAPIENGINE_LMCUTLANDMARK_H
