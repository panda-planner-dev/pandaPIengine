//
// Created by dh on 28.02.20.
//

#include "LmAoNode.h"

int LmAoNode::getSize() {
    if(this->containsFullSet)
        return maxSize;
    else
        return numLMs;

}

bool LmAoNode::contains(int lm) {
    if(this->containsFullSet)
        return true;
    else {
        return iu.containsInt(this->lms, 0, this->numLMs - 1, lm);
    }
}

LmAoNode::LmAoNode(int ownIndex) {
    this->ownIndex = ownIndex;
}

void LmAoNode::print() {
    cout << "node" << this->ownIndex;
    if(this->nodeType == OR)
        cout << " OR   ";
    else if(this->nodeType == AND)
        cout << " AND  ";
    else
        cout << " INIT ";
    if(this->containsFullSet){
        cout << "LM set = FULL";
    } else {
        cout << "LM set = {";
        for (int i = 0; i < this->numLMs; i++) {
            if (i > 0) {
                cout << " ";
            }
            cout << this->lms[i];
        }
        cout << "}";
    }
}
