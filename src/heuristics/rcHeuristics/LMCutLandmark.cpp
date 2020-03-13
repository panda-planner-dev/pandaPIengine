//
// Created by dh on 12.03.20.
//

#include "LMCutLandmark.h"

bool progression::LMCutLandmark::isMethod(int i) {
    return (i >= firstMethod);
}

bool progression::LMCutLandmark::isAction(int i) {
    return (i < firstMethod);
}

progression::LMCutLandmark::LMCutLandmark(int size) {
    this->size = size;
    this->lm = new int[size];
}
