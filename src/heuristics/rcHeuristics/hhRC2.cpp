//
// Created by dh on 10.03.20.
//

#include "hhRC2.h"
hhRC2::hhRC2(Model *htnModel) {

    Model* heuristicModel;
    factory = new RCModelFactory(htnModel);
    heuristicModel = factory->getRCmodelSTRIPS();

#if HEURISTIC == RCFF2
    this->sasH = new hsAddFF(heuristicModel);
    this->sasH->heuristic = sasFF;
#elif HEURISTIC == RCADD2
    this->sasH = new hsAddFF(heuristicModel);
	this->sasH->heuristic = sasAdd;
#elif HEURISTIC == RCLMC2
    this->sasH = new hsLmCut(heuristicModel);
#elif HEURISTIC == RCFILTER2
    this->sasH = new hsFilter(heuristicModel);
#endif

#ifdef RCLMC2STORELMS
    this->sasH = new hsLmCut(heuristicModel);
#endif

    htn = htnModel;
    intSet.init(sasH->m->numStateBits);
    gset.init(sasH->m->numStateBits);
    s0set.init(sasH->m->numStateBits);

#ifdef CORRECTTASKCOUNT
	htnModel->calcMinimalImpliedX();
#endif
}

hhRC2::~hhRC2() {
    delete factory;
}

int hhRC2::setHeuristicValue(searchNode *n) {
    int hval = 0;

    // get facts holding in s0
    s0set.clear();
    for (int i = 0; i < htn->numStateBits; i++) {
        if (n->state[i]) {
            s0set.insert(i);
        }
    }

    // add reachability facts and HTN-related goal
    for (int i = 0; i < n->numAbstract; i++) {
        // add reachability facts
        for (int j = 0; j < n->unconstraintAbstract[i]->numReachableT; j++) {
            int t = n->unconstraintAbstract[i]->reachableT[j];
            if (t < htn->numActions)
                s0set.insert(factory->t2tdr(t));
        }
    }
    for (int i = 0; i < n->numPrimitive; i++) {
        // add reachability facts
        for (int j = 0; j < n->unconstraintPrimitive[i]->numReachableT; j++) {
            int t = n->unconstraintPrimitive[i]->reachableT[j];
            if (t < htn->numActions)
                s0set.insert(factory->t2tdr(t));
        }
    }

    // generate goal
    gset.clear();
    for (int i = 0; i < htn->gSize; i++) {
        gset.insert(htn->gList[i]);
    }

    for(int i = 0; i < n->numContainedTasks; i++) {
        int t = n->containedTasks[i];
        gset.insert(factory->t2bur(t));
    }

    hval = this->sasH->getHeuristicValue(s0set, gset);

#ifdef RCLMC2STORELMS
    // the indices of the methods need to be transformed to fit the scheme of the HTN model (as opposed to the rc model)
    if((storeCuts) && (hval != UNREACHABLE)) {
        this->cuts = this->sasH->cuts;
        for (LMCutLandmark* storedcut : *cuts) {
            iu.sort(storedcut->lm, 0, storedcut->size - 1);
            /*for (int i = 0; i < storedcut->size; i++) {
                if ((i > 0) && (storedcut->lm[i] >= htn->numActions) && (storedcut->lm[i - 1] < htn->numActions))
                    cout << "| ";
                cout << storedcut->lm[i] << " ";
            }
            cout << "(there are " << htn->numActions << " actions)" << endl;*/

            // looking for index i s.t. lm[i] is a method and lm[i - 1] is an action
            int leftmostMethod;
            if(storedcut->lm[0] >= htn->numActions) {
                leftmostMethod = 0;
            } else if (storedcut->lm[storedcut->size - 1] < htn->numActions) {
                leftmostMethod = storedcut->size;
            } else { // there is a border between actions and methods
                int rightmostAction = 0;
                leftmostMethod = storedcut->size - 1;
                while (rightmostAction != (leftmostMethod - 1)) {
                    int middle = (rightmostAction + leftmostMethod) / 2;
                    if (storedcut->lm[middle] < htn->numActions){
                        rightmostAction = middle;
                    } else {
                        leftmostMethod = middle;
                    }
                }
            }
            storedcut->firstMethod = leftmostMethod;
            /*if (leftmostMethod > storedcut->size -1)
                cout << "leftmost method: NO METHOD" << endl;
            else
                cout << "leftmost method: " << leftmostMethod << endl;*/
            for (int i = storedcut->firstMethod; i < storedcut->size; i++) {
                storedcut->lm[i] -= htn->numActions; // transform index
            }
        }
    }

    /*
    for(LMCutLandmark* lm :  *cuts) {
        cout << "cut: {";
        for (int i = 0; i < lm->size; i++) {
            if(lm->isAction(i)) {
                cout << this->htn->taskNames[lm->lm[i]];
            } else {
                cout << this->htn->methodNames[lm->lm[i]];
            }
            if (i < lm->size - 1) {
                cout << ", ";
            }
        }
        cout << "}" << endl;
    }*/
#endif

#ifdef CORRECTTASKCOUNT
    if (hval != UNREACHABLE) {
        for (int i = 0; i < n->numContainedTasks; i++) {
            if (n->containedTaskCount[i] > 1) {
                int task = n->containedTasks[i];
                int count = n->containedTaskCount[i];
                hval += (htn->minImpliedDistance[task] * (count - 1));
            }
        }
    }
#endif
    return hval;
}

void hhRC2::setHeuristicValue(searchNode *n, searchNode *parent, int action) {
    n->heuristicValue = this->setHeuristicValue(n);
    n->goalReachable = (n->heuristicValue != UNREACHABLE);
}

void hhRC2::setHeuristicValue(searchNode *n, searchNode *parent, int absTask, int method) {
    n->heuristicValue = this->setHeuristicValue(n);
    n->goalReachable = (n->heuristicValue != UNREACHABLE);
}