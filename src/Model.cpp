/*
 * HtnModel.cpp
 *
 *  Created on: 05.09.2017
 *      Author: Daniel Höller
 */

#include "Model.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <utility>
#include <cassert>
#include <list>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <map>
#include <sys/time.h>
#include "intDataStructures/IntPairHeap.h"

using namespace std;

namespace progression {

Model::Model() {
    numMethodsTrans = 0;
    firstMethodIndex = 0;
    numStateBits = 0;
    numTasks = 0;
    numPrecLessActions = 0;
    numMethods = 0;
    initialTask = -1;
    gSize = 0;
    isHtnModel = false;
    numActions = 0;
    numVars = 0;
    s0Size = 0;
    numVarsTrans = 0;
    headIndex = 0;
    firstTaskIndex = 0;
    firstVarIndex = 0;
    numStateBitsTrans = 0;
    s0SizeTrans = 0;
    numActionsTrans = 0;
    numInvalidTransActions = 0;
    numStateBitsSP = 0;
	  numEmptyTasks = 0;
	  firstEmptyTaskIndex = 0;
	  emptyTaskNames = nullptr;
	  numEmptyTaskPrecs = nullptr;
    numEmptyTaskAdds = nullptr;
	  emptyTaskPrecs = nullptr;
	  emptyTaskAdds = nullptr;
    bitAlone = nullptr;
    sasPlusBits = nullptr;
    sasPlusOffset = nullptr;
	  factStrsSP = nullptr;
	  firstIndexSP = nullptr;
	  lastIndexSP = nullptr;
	  bitsToSP = nullptr;
	  strictMutexesSP = nullptr;
	
	  mutexesSP = nullptr;

	  precListsSP = nullptr;
	  addListsSP = nullptr;

	  numPrecsSP = nullptr;
	  numAddsSP = nullptr;

	  s0ListSP = nullptr;
	  gListSP = nullptr;
	  gListTrans = nullptr;
	  gSizeTrans = 0;

    numConditionalEffectsTrans = nullptr;
    effectConditionsTrans = nullptr;
    numEffectConditionsTrans = nullptr;
    effectsTrans = nullptr;
    taskToKill = nullptr;
    firstConstraintIndex = 0;
    methodIndexes = nullptr;
    subTasksInOrder = nullptr;
    invalidTransActions = nullptr;
    actionCostsTrans = nullptr;
    precListsTrans = nullptr;
    addListsTrans = nullptr;
    delListsTrans = nullptr;
    numPrecsTrans = nullptr;
    numAddsTrans = nullptr;
    numDelsTrans = nullptr;
    firstNumTOPrimTasks = nullptr;
    s0ListTrans = nullptr;
    factStrsTrans = nullptr;
    firstIndexTrans = nullptr;
    lastIndexTrans = nullptr;
    varNamesTrans = nullptr;
    factStrs = nullptr;
    actionNamesTrans = nullptr;
    firstIndex = nullptr;
    lastIndex = nullptr;
    varNames = nullptr;
    actionCosts = nullptr;
    precLists = nullptr;
    addLists = nullptr;
    delLists = nullptr;
    precLessActions = nullptr;
    precToActionSize = nullptr;
    precToAction = nullptr;
    numPrecs = nullptr;
    numAdds = nullptr;
    numDels = nullptr;
    s0List = nullptr;
    gList = nullptr;
    isPrimitive = nullptr;
    taskNames = nullptr;
    decomposedTask = nullptr;
    numSubTasks = nullptr;
    numFirstPrimSubTasks = nullptr;
    numFirstAbstractSubTasks = nullptr;
    numOrderings = nullptr;
    methodNames = nullptr;
    numFirstTasks = nullptr;
    numLastTasks = nullptr;
    taskToMethods = nullptr;
    numMethodsForTask = nullptr;
    subTasks = nullptr;
    ordering = nullptr;
    methodsFirstTasks = nullptr;
    methodsLastTasks = nullptr;
    methodSubtaskSuccNum = nullptr;
#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
    addVectors = nullptr;
    delVectors = nullptr;
    s0Vector = nullptr;
#endif

#ifdef PRGEFFECTLESS
    effectLess = new FlexIntStack();
    effectLess->init(25);
#endif
#ifdef ONEMODMETH
    oneMod = new FlexIntStack();
    oneMod->init(25);
#endif

    /*
#ifdef TRACKLMSFULL
    newlyReachedFLMs = new noDelIntSet();
    newlyReachedTLMs = new noDelIntSet();
    newlyReachedMLMs = new noDelIntSet();
    newlyReachedFLMs->init(tn);
    newlyReachedTLMs = new noDelIntSet();
    newlyReachedMLMs = new noDelIntSet();
#endif
*/

}

Model::~Model() {
    for (int i = 0; i < numMethods; i++){
        delete[] subTasksInOrder[i];
    }
    for (int i = 0; i < numActionsTrans; i++){
      for (int j= 0; j < numConditionalEffectsTrans[i]; j++){
        delete[] effectConditionsTrans[i][j];
      }
      delete[] numEffectConditionsTrans[i];
      delete[] effectsTrans[i];
    }
    for (int i = 0; i < numEmptyTasks; i++){
      delete[] emptyTaskPrecs;
      delete[] emptyTaskAdds;
    }
    delete[] firstNumTOPrimTasks;
    delete[] numEmptyTaskPrecs;
    delete[] emptyTaskNames;
    delete[] numEmptyTaskAdds;
    delete[] bitAlone;
    delete[] bitsToSP;
    delete[] sasPlusBits;
    delete[] sasPlusOffset;
    delete[] factStrsSP;
    delete[] firstIndexSP;
    delete[] lastIndexSP;
    delete[] numConditionalEffectsTrans;
    delete[] methodIndexes;
    delete[] taskToKill;
    delete[] subTasksInOrder;
    delete[] invalidTransActions;
    delete[] actionNamesTrans;
    delete[] s0ListTrans;
    delete[] factStrs;
    delete[] firstIndex;
    delete[] lastIndex;
    delete[] varNames;
    delete[] actionCosts;
    delete[] lastIndexTrans;
    delete[] varNamesTrans;
    delete[] factStrsTrans;
    delete[] actionCostsTrans;
    delete[] precListsTrans;
    delete[] addListsTrans;
    delete[] delListsTrans;
    delete[] gListTrans;
    for (int i = 0; i < numActionsTrans; i++) {
        delete[] precListsTrans[i];
        delete[] addListsTrans[i];
        delete[] delListsTrans[i];
    }
    delete[] numPrecsTrans;
    delete[] numAddsTrans;
    delete[] numDelsTrans;
    for (int i = 0; i < numActions; i++) {
        delete[] precLists[i];
        delete[] addLists[i];
        delete[] precListsSP[i];
        delete[] addListsSP[i];
        delete[] delLists[i];
    }
    delete[] precLists;
    delete[] addLists;
    delete[] delLists;
    delete[] precListsSP;
    delete[] addListsSP;
    delete[] precLessActions;
    delete[] precToActionSize;
    for (int i = 0; i < numStateBits; i++) {
        delete[] precToAction[i];
    }
    delete[] precToAction;
    delete[] numPrecs;
    delete[] numAdds;
    delete[] numDels;
    delete[] numPrecsSP;
    delete[] numAddsSP;
    delete[] s0List;
    delete[] gList;
    delete[] s0ListSP;
	  delete[] gListSP;
    delete[] isPrimitive;
    delete[] taskNames;
#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
    for (int i = 0; i < numActions; i++) {
        delete[] addVectors[i];
        delete[] delVectors[i];
    }
    delete[] addVectors;
    delete[] delVectors;
    delete[] s0Vector;
#endif

    if (!isHtnModel)
        return;
    delete[] decomposedTask;
    delete[] numSubTasks;
    delete[] numFirstPrimSubTasks;
    delete[] numFirstAbstractSubTasks;
    delete[] numOrderings;
    delete[] methodNames;
    delete[] numFirstTasks;
    delete[] numLastTasks;
    for (int i = 0; i < numTasks; i++) {
        delete[] taskToMethods[i];
    }
    delete[] taskToMethods;
    delete[] numMethodsForTask;
    for (int i = 0; i < numMethods; i++) {
        delete[] subTasks[i];
        delete[] ordering[i];
        delete[] methodsFirstTasks[i];
        delete[] methodsLastTasks[i];
    }
    delete[] subTasks;
    delete[] ordering;
    delete[] methodsFirstTasks;
    delete[] methodsLastTasks;
    for (int i = 0; i < numMethods; i++)
        delete[] methodSubtaskSuccNum[i];
    delete[] methodSubtaskSuccNum;
    if (calculatedSccs) {
        delete[] sccGnumSucc;
        for (int i = 0; i < numSCCs; i++) {
            delete[] sccG[i];
        }
        delete[] sccG;
        delete[] sccGnumPred;
        for (int i = 0; i < numSCCs; i++) {
            delete[] sccGinverse[i];
        }
        delete[] sccGinverse;

        delete[] numReachable;
        for (int i = 0; i < numTasks; i++) {
            delete[] reachable[i];
        }
        delete[] reachable;
    }
    delete[] sccsCyclic;
    delete[] taskToSCC;
    delete[] sccSize;
    if (sccToTasks != nullptr) {
        for (int i = 0; i < numSCCs; i++)
            delete[] sccToTasks[i];
        delete[] sccToTasks;
    }
#ifdef PRGEFFECTLESS
    delete effectLess;
#endif
#ifdef ONEMODMETH
    delete oneMod;
#endif

    delete[] numDistinctSTs;
    for(int i = 0; i < numMethods; i++) {
        delete[] sortedDistinctSubtasks[i];
        delete[] sortedDistinctSubtaskCount[i];
    }
    delete[] sortedDistinctSubtasks;
    delete[] sortedDistinctSubtaskCount;
    for (int i =0; i < numTasks; i++){
        delete[] stToMethod[i];
    }
    delete[] stToMethodNum;
    delete[] stToMethod;

#ifdef CALCMINIMALIMPLIEDCOSTS
    delete[] minImpliedCosts;
    delete[] minImpliedDistance;
#endif

}

#ifdef TRACKTASKSINTN

void Model::updateTaskCounterA(searchNode* n, searchNode* parent, int action) {
    int ai = iu.indexOf(parent->containedTasks, 0, parent->numContainedTasks - 1, action);
    assert(ai >= 0); //should be in there...
    assert(ai < parent->numContainedTasks);
    assert(parent->containedTaskCount[ai] > 0);
    bool removeAI = (parent->containedTaskCount[ai] == 1);
    n->numContainedTasks = parent->numContainedTasks;
    if (removeAI) {
        n->numContainedTasks--;
    }
    n->containedTasks = new int[n->numContainedTasks];
    n->containedTaskCount = new int[n->numContainedTasks];
    for (int i = 0; i < ai; i++) {
        n->containedTasks[i] = parent->containedTasks[i];
        n->containedTaskCount[i] = parent->containedTaskCount[i];
    }
    int ni = ai;
    if (!removeAI) {
        n->containedTasks[ni] = parent->containedTasks[ni];
        n->containedTaskCount[ni] = parent->containedTaskCount[ni] - 1;
        ni++;
    }
    for (int i = ai + 1; i < parent->numContainedTasks; i++) {
        n->containedTasks[ni] = parent->containedTasks[i];
        n->containedTaskCount[ni] = parent->containedTaskCount[i];
        ni++;
    }

#ifndef NDEBUG
/*
    cout << endl << endl << "TN n:" << n << " Parent " << parent << endl;
    cout << "applied action " << action << endl;
    cout << "old ";
    for(int i = 0;i < parent->numContainedTasks;i++) {
        cout << parent->containedTasks[i] << "*" << parent->containedTaskCount[i] << " ";
    }
    cout << endl;
    cout << "new ";
    for(int i = 0;i < n->numContainedTasks;i++) {
        cout << n->containedTasks[i] << "*" << n->containedTaskCount[i] << " ";
    }
    cout << endl << endl;

*/
    // count tasks in this network and compare to the tracked values
/*  int* counted = new int[this->numTasks];
    for (int i = 0; i < this->numTasks; i++)
        counted[i] = 0;
    set<int> done;
    vector<planStep*> todoList;
    for (int i = 0; i < n->numPrimitive; i++)
        todoList.push_back(n->unconstraintPrimitive[i]);
    for (int i = 0; i < n->numAbstract; i++)
        todoList.push_back(n->unconstraintAbstract[i]);
*/
/*  for (int i = 0; i < n->numPrimitive; i++)
        cout << "UCP: " << n->unconstraintPrimitive[i] << endl;
    for (int i = 0; i < n->numAbstract; i++)
        cout << "UCA: " << n->unconstraintAbstract[i] << endl;

    for (int i = 0; i < parent->numPrimitive; i++)
        cout << "UCP-P: " << parent->unconstraintPrimitive[i] << endl;
    for (int i = 0; i < parent->numAbstract; i++)
        cout << "UCA-P: " << parent->unconstraintAbstract[i] << endl;
*/

/*  while (!todoList.empty()) {
        planStep* ps = todoList.back();
        //cout << "I reach " << ps << " " << ps->task << endl;
        todoList.pop_back();
        if (done.count(ps->id)) continue;
        done.insert(ps->id);
        counted[ps->task]++;
        for (int i = 0; i < ps->numSuccessors; i++) {
            planStep* succ = ps->successorList[i];
            //cout << "I-S " << succ << " " << ps << endl;
            const bool included = done.find(succ->id) != done.end();
            if (!included)
                todoList.push_back(succ);
        }
    }
*/

/*  // count tasks in this network and compare to the tracked values
            int* counted2 = new int[this->numTasks];
            for (int i = 0; i < this->numTasks; i++)
                counted2[i] = 0;
            set<int> done2;
            vector<planStep*> todoList2;
            for (int i = 0; i < parent->numPrimitive; i++)
                todoList2.push_back(parent->unconstraintPrimitive[i]);
            for (int i = 0; i < parent->numAbstract; i++)
                todoList2.push_back(parent->unconstraintAbstract[i]);
            while (!todoList2.empty()) {
                planStep* ps = todoList2.back();
                cout << "Parent reach " << ps << " " << ps->task << endl;
                todoList2.pop_back();
                if (done2.count(ps->id)) continue;
                done2.insert(ps->id);
                counted2[ps->task]++;
                for (int i = 0; i < ps->numSuccessors; i++) {
                    planStep* succ = ps->successorList[i];
                    cout << "P-S " << succ << " " << ps << endl;
                    const bool included = done2.find(succ->id) != done2.end();
                    if (!included)
                        todoList2.push_back(succ);
                }
            }
*/
/*  map<int,int> containedTasksUpdated;
    for (int i = 0; i < n->numContainedTasks; i++)
        containedTasksUpdated[n->containedTasks[i]] = n->containedTaskCount[i];
*/
/*  //for(int i = 0; i < this->numTasks; i++) {
        //if(counted[i] != containedTasksUpdated [i]) {
            cout << "TN n:" << n << " Parent " << parent << endl;
            cout << endl << "Used action: " << " " << action << " " << this->taskNames[action] << endl << endl;
            for(int j = 0; j < this->numTasks; j++)
                if((counted[j] + containedTasksUpdated [j]) > 0)
                    cout << j << " " << this->taskNames[j] << " " << counted[j] << " " << containedTasksUpdated [j] << endl;

            cout << endl << "solution:" << endl;
            solutionStep* s = n->solution;
            while (s){
                cout << this->taskNames[s->task] << endl;
                s = s->prev;
            }







            cout << "OLD" << endl;
            for(int j = 0; j < this->numTasks; j++)
                if((counted2[j]) > 0)
                    cout << j << " " << this->taskNames[j] << " " << counted2[j] << endl;


            //if (this->taskNames[action] == "__method_precondition_m-get_image_data[camera0,high_res,objective1,rover0,waypoint0]") exit(1);

*/

        //}
//  for(int i = 0; i < this->numTasks; i++)
//      assert(counted[i] == containedTasksUpdated[i]);

#endif
}

void Model::updateTaskCounterM(searchNode* n, searchNode* parent, int m) {

    /*cout << "applied method " << m << " -> "  << decomposedTask[m] << " " << " (";
    for(int i = 0; i < this->numSubTasks[m]; i++)
        cout << this->subTasks[m][i] << " ";
    cout << " aka ";
    for(int i = 0; i < this->numDistinctSTs[m]; i++)
        cout << this->sortedDistinctSubtasks[m][i] << " ";
    cout << ")" << endl;
    cout << "old ";
    for(int i = 0;i < parent->numContainedTasks;i++) {
        cout << parent->containedTasks[i] << "*" << parent->containedTaskCount[i] << " ";
    }
    cout << endl;*/

    int newElements = 0;
    int low = 0;
    for(int i = 0; i < this->numDistinctSTs[m]; i++) {
        int st = this->sortedDistinctSubtasks[m][i];
        int index = iu.indexOf(parent->containedTasks, low, parent->numContainedTasks - 1, st);
        if (index >= 0) {
            low = index + 1;
        } else {
            newElements++;
        }
    }

    int mAbs = this->decomposedTask[m];
    int index = iu.indexOf(parent->containedTasks, 0, parent->numContainedTasks - 1, mAbs);
    assert(index >= 0);
    assert(parent->containedTaskCount[index] > 0);

    n->numContainedTasks = parent->numContainedTasks + newElements;
    bool removeAbs = ((parent->containedTaskCount[index] == 1)
            && (iu.indexOf(sortedDistinctSubtasks[m], 0, numDistinctSTs[m] - 1, mAbs) < 0));
    if (removeAbs) {
        n->numContainedTasks--;
    }

    n->containedTasks = new int[n->numContainedTasks];
    n->containedTaskCount = new int[n->numContainedTasks];

    int itn = 0;
    int im = 0;
    int ir = 0;

    while((itn < parent->numContainedTasks) || (im < numDistinctSTs[m])) {
        if ((itn < parent->numContainedTasks) && (im < numDistinctSTs[m])){
            if((removeAbs) && (parent->containedTasks[itn] == mAbs)) {
                itn++;
                continue;
            } else if ((removeAbs) && (sortedDistinctSubtasks[m][im] == mAbs)) {
                im++;
                continue;
            }

            if(parent->containedTasks[itn] < sortedDistinctSubtasks[m][im]) {
                n->containedTasks[ir] = parent->containedTasks[itn];
                n->containedTaskCount[ir] = parent->containedTaskCount[itn];
                itn++;
                ir++;
            } else if (parent->containedTasks[itn] > sortedDistinctSubtasks[m][im]) {
                n->containedTasks[ir] = sortedDistinctSubtasks[m][im];
                n->containedTaskCount[ir] = sortedDistinctSubtaskCount[m][im];
                im++;
                ir++;
            } else { // equal
                n->containedTasks[ir] = sortedDistinctSubtasks[m][im];
                n->containedTaskCount[ir] = sortedDistinctSubtaskCount[m][im];
                n->containedTaskCount[ir] += parent->containedTaskCount[itn];
                itn++;
                im++;
                ir++;
            }
        } else if (itn < parent->numContainedTasks) {
            if((removeAbs) && (parent->containedTasks[itn] == mAbs)) {
                itn++;
                continue;
            }
            n->containedTasks[ir] = parent->containedTasks[itn];
            n->containedTaskCount[ir] = parent->containedTaskCount[itn];
            itn++;
            ir++;
        } else if (im < numDistinctSTs[m]) {
            if((removeAbs) && (sortedDistinctSubtasks[m][im] == mAbs)) {
                im++;
                continue;
            }
            n->containedTasks[ir] = sortedDistinctSubtasks[m][im];
            n->containedTaskCount[ir] = sortedDistinctSubtaskCount[m][im];
            im++;
            ir++;
        }
    }
    if (!removeAbs) {
        index = iu.indexOf(n->containedTasks, 0, n->numContainedTasks - 1, mAbs);
        n->containedTaskCount[index]--;
        assert(n->containedTaskCount[index] > 0);
    }

    /*cout << "new ";
    for(int i = 0;i < n->numContainedTasks;i++) {
        cout << n->containedTasks[i] << "*" << n->containedTaskCount[i] << " ";
    }
    cout << endl << endl;*/

#ifndef NDEBUG
/*  for (int i = 0; i < n->numPrimitive; i++)
        cout << "UCP: " << n->unconstraintPrimitive[i] << endl;
    for (int i = 0; i < n->numAbstract; i++)
        cout << "UCA: " << n->unconstraintAbstract[i] << endl;

    for (int i = 0; i < parent->numPrimitive; i++)
        cout << "UCP-P: " << parent->unconstraintPrimitive[i] << endl;
    for (int i = 0; i < parent->numAbstract; i++)
        cout << "UCA-P: " << parent->unconstraintAbstract[i] << endl;
*/

    // count tasks in this network and compare to the tracked values
/*  int* counted = new int[this->numTasks];
    for (int i = 0; i < this->numTasks; i++)
        counted[i] = 0;
    set<int> done;
    vector<planStep*> todoList;
    for (int i = 0; i < n->numPrimitive; i++)
        todoList.push_back(n->unconstraintPrimitive[i]);
    for (int i = 0; i < n->numAbstract; i++)
        todoList.push_back(n->unconstraintAbstract[i]);
    while (!todoList.empty()) {
        planStep* ps = todoList.back();
        todoList.pop_back();
        if (done.count(ps->id)) continue;
        //cout << "I reach " << ps << " " << ps->task << endl;
        done.insert(ps->id);
        counted[ps->task]++;
        for (int i = 0; i < ps->numSuccessors; i++) {
            planStep* succ = ps->successorList[i];
            const bool included = done.find(succ->id) != done.end();
            if (!included)
                todoList.push_back(succ);
        }
    }



            // count tasks in this network and compare to the tracked values
            int* counted2 = new int[this->numTasks];
            for (int i = 0; i < this->numTasks; i++)
                counted2[i] = 0;
            set<int> done2;
            vector<planStep*> todoList2;
            for (int i = 0; i < parent->numPrimitive; i++)
                todoList2.push_back(parent->unconstraintPrimitive[i]);
            for (int i = 0; i < parent->numAbstract; i++)
                todoList2.push_back(parent->unconstraintAbstract[i]);
            while (!todoList2.empty()) {
                planStep* ps = todoList2.back();
                todoList2.pop_back();
        if (done2.count(ps->id)) continue;
                //cout << "Parent reach " << ps << " " << ps->task << endl;
                done2.insert(ps->id);
                counted2[ps->task]++;
                for (int i = 0; i < ps->numSuccessors; i++) {
                    planStep* succ = ps->successorList[i];
                    const bool included = done2.find(succ->id) != done2.end();
                    if (!included)
                        todoList2.push_back(succ);
                }
            }

    map<int,int> containedTasksUpdated;
    for (int i = 0; i < n->numContainedTasks; i++)
        containedTasksUpdated[n->containedTasks[i]] = n->containedTaskCount[i];
*/
/*  //for(int i = 0; i < this->numTasks; i++) {
        //if(counted[i] != containedTasksUpdated [i]) {
            cout << "TN n:" << n << " Parent " << parent << endl;
            cout << endl << "Used method: " << " " << m << " " << this->methodNames[m] << endl << endl;
            for(int j = 0; j < this->numTasks; j++)
                if((counted[j] + containedTasksUpdated [j]) > 0)
                    cout << j << " " << this->taskNames[j] << " " << counted[j] << " " << containedTasksUpdated [j] << endl;

            cout << endl << "solution:" << endl;
            solutionStep* s = n->solution;
            while (s){
                cout << this->taskNames[s->task] << endl;
                s = s->prev;
            }



    for (int i = 0; i < parent->numPrimitive; i++)
        cout << "UCP: " << parent->unconstraintPrimitive[i] << endl;
    for (int i = 0; i < parent->numAbstract; i++)
        cout << "UCA: " << parent->unconstraintAbstract[i] << endl;


            cout << "OLD" << endl;
            for(int j = 0; j < this->numTasks; j++)
                if((counted2[j]) > 0)
                    cout << j << " " << this->taskNames[j] << " " << counted2[j] << endl;


*/


        //}
//  for(int i = 0; i < this->numTasks; i++)
//      assert(counted[i] == containedTasksUpdated[i]);
    //}
//  delete[] counted;

#endif
}

#endif

searchNode* Model::decompose(searchNode *n, int taskNo, int method) {
    planStep *decomposed = n->unconstraintAbstract[taskNo];
    assert(!isPrimitive[decomposed->task]);
    assert(decomposedTask[method] == decomposed->task);

#ifdef TRACESOLUTION
    int mySolutionStepInstanceNumber = progression::currentSolutionStepInstanceNumber++;
#endif

    searchNode* result = new searchNode;
#if STATEREP == SRCOPY
    result->state = n->state;
#elif STATEREP == SRLIST
    result->stateSize = n->stateSize;
    result->state = new int[n->stateSize];
    for (int i = 0; i < n->stateSize; i++)
    result->state[i] = n->state[i];
#endif
    // prepare data structures
    result->numPrimitive = n->numPrimitive + numFirstPrimSubTasks[method];
    if (result->numPrimitive > 0) {
        result->unconstraintPrimitive = new planStep*[result->numPrimitive];
    } else {
        result->unconstraintPrimitive = nullptr;
    }

    result->numAbstract = n->numAbstract + numFirstAbstractSubTasks[method] - 1; // subtract the decomposed one
    if (result->numAbstract > 0) {
        result->unconstraintAbstract = new planStep*[result->numAbstract];
    } else {
        result->unconstraintAbstract = nullptr;
    }

    // fill data structures
#ifdef ONEMODMETH
    oneMod->clear();
#endif
    for (int i = 0; i < n->numPrimitive; i++) {
        result->unconstraintPrimitive[i] = n->unconstraintPrimitive[i];
    }

    int current = 0;
    for (int i = 0; i < n->numAbstract; i++) {
        if (i != taskNo) {
            result->unconstraintAbstract[current] = n->unconstraintAbstract[i];
#ifdef ONEMODMETH
            if (numMethodsForTask[result->unconstraintAbstract[current]->task]
                    == 1)
                oneMod->push(current);
#endif
            current++;
        }
    }

    // add the successors of the decomposed task as successor of the method's last tasks
    pair<planStep**, planStep**> mInstance = initializeMethod(method
#ifdef TRACESOLUTION
            , mySolutionStepInstanceNumber
#endif
            ); // returns first and last tasks
    for (int i = 0; i < numLastTasks[method]; i++) {
        mInstance.second[i]->successorList =
                new planStep*[decomposed->numSuccessors];
        mInstance.second[i]->numSuccessors = decomposed->numSuccessors;
        for (int j = 0; j < decomposed->numSuccessors; j++) {
            mInstance.second[i]->successorList[j] =
                    decomposed->successorList[j];
        }
    }

#ifdef PRGEFFECTLESS
    effectLess->clear();
#endif

    int primI = n->numPrimitive;
    int absI = n->numAbstract - 1;
    for (int i = 0; i < numFirstTasks[method]; i++) {
        if (isPrimitive[mInstance.first[i]->task]) {
            result->unconstraintPrimitive[primI] = mInstance.first[i];
#ifdef PRGEFFECTLESS
            if ((this->numAdds[result->unconstraintPrimitive[primI]->task] == 0)
                    && (this->numDels[result->unconstraintPrimitive[primI]->task]
                            == 0)
                    && (this->isApplicable(result,
                            result->unconstraintPrimitive[primI]->task))) {
                effectLess->push(primI);
            }
#endif
            primI++;
        } else {
            result->unconstraintAbstract[absI] = mInstance.first[i];
#ifdef ONEMODMETH
            if (numMethodsForTask[result->unconstraintAbstract[absI]->task]
                    == 1)
                oneMod->push(absI);
#endif
            absI++;
        }
    }
    delete[] mInstance.first;
    delete[] mInstance.second;

    assert(primI == result->numPrimitive);
    assert(absI == result->numAbstract);

    result->solution = new solutionStep();
    result->solution->task = decomposed->task;
    result->solution->method = method;
    result->solution->prev = n->solution;
#ifdef TRACESOLUTION
    result->solution->mySolutionStepInstanceNumber = mySolutionStepInstanceNumber;
    result->solution->myPositionInParent = decomposed->myPositionInParent;
    result->solution->parentSolutionStepInstanceNumber = decomposed->parentSolutionStepInstanceNumber;
#endif
    result->solution->pointersToMe = 1;
    result->modificationDepth = n->modificationDepth + 1;
    result->mixedModificationDepth = n->mixedModificationDepth + 1;
    result->actionCosts = n->actionCosts;

    // maintain pointer counter
    for (int i = 0; i < result->numPrimitive; i++) {
        result->unconstraintPrimitive[i]->pointersToMe++;
    }
    for (int i = 0; i < result->numAbstract; i++) {
        result->unconstraintAbstract[i]->pointersToMe++;
    }
    for (int i = 0; i < decomposed->numSuccessors; i++) {
        planStep* succ = decomposed->successorList[i];
        succ->pointersToMe += numLastTasks[method];
    }
    if (n->solution != nullptr)
        n->solution->pointersToMe++;

#ifdef TRACKTASKSINTN
    updateTaskCounterM(result, n, method);
#endif

#ifdef MAINTAINREACHABILITY
    updateReachability(result);
#endif

#ifdef TRACKLMSFULL
    /*cout << "decomposition" << endl;
    if(first) {
        for(int i = 0; i < n->numLMs; i++) {
            cout << i << " ";
            n->lms[i]->printLM();
        }
        first = false;
    }*/

    result->lms = n->lms;
    result->numLMs = n->numLMs;
    result->lookForF = n->lookForF;
    result->lookForF->refCounter++;
    result->lookForM = n->lookForM;
    result->lookForM->refCounter++;

    set<int> newlyReachedT;
    for(int iST = 0; iST < this->numSubTasks[method]; iST++) {
        int st = this->subTasks[method][iST];
        if(!iu.containsInt(n->containedTasks, 0, n->numContainedTasks - 1, st)) {
             // this task is newly inserted
            int iLmMapping = n->lookForT->indexOf(st);
            if (iLmMapping >= 0) { // check LMs for this task
                for(int iLMM = 0; iLMM < n->lookForT->lookFor[iLmMapping]->size; iLMM++){
                    int iLMglobal = n->lookForT->lookFor[iLmMapping]->containedInLMs[iLMM];
                    if(newlyReachedT.find(iLMglobal) != newlyReachedT.end())
                        continue;
                    landmark* lm = n->lms[iLMglobal];
                    if((lm->connection == atom) || (lm->connection == disjunctive)) {
                        newlyReachedT.insert(iLMglobal);
                    } else if(lm->connection == conjunctive) {
                        // more than one task contained in the lm might be in the current method
                        // therefore we use the "result" object for the following check
                        bool fulfilled = true;
                        for(int iCon = 0; iCon < lm->size; iCon++) {
                            int tCon = lm->lm[iCon];
                            if(!iu.containsInt(result->containedTasks, 0, result->numContainedTasks - 1, tCon)){
                                fulfilled = false;
                                break;
                            }
                        }
                        if(fulfilled){
                            newlyReachedT.insert(iLMglobal);
                        }
                    }
                }
            }
        }
    }
    if(newlyReachedT.size() > 0) {
        //n->lookForT->printTab();

        /*
        cout << "subtasks ";
        for(int iST = 0; iST < this->numSubTasks[method]; iST++) {
            cout << this->subTasks[method][iST] << " ";
        }
        cout << endl;*/
        // copy lm data, delete reached lms from "lookFor" lists
        set<int> needToModify; // these are the tasks that need to be modified
        for(int lmID : newlyReachedT) {
            landmark* lm = n->lms[lmID];
            //cout << lmID << " ";
            //lm->printLM();
            for(int iT = 0; iT < lm->size; iT++) {
                needToModify.insert(lm->lm[iT]);
            }
        }

        /*
        cout << "Need to modify ";
        for(int e : needToModify) {
            cout << e << " ";
        }
        cout << endl;*/

        set<int>::iterator it = needToModify.begin();
        int copyI = 0;
        int deleted = 0;
        result->lookForT = new lookUpTab(n->lookForT->size);
        while (it != needToModify.end()) {
            int taskToModify = (*it);
            //cout << "taskToModify " << taskToModify << endl;
            // copy until finding a task with a mapping that needs to be modified
            while(n->lookForT->lookFor[copyI]->entry < taskToModify) {
                result->lookForT->lookFor[copyI - deleted] = n->lookForT->lookFor[copyI];
                //cout << "copy " << n->lookForT->lookFor[copyI]->entry << endl;
                copyI++;
            }
            assert(taskToModify == n->lookForT->lookFor[copyI]->entry);

            // modify the entry
            result->lookForT->lookFor[copyI - deleted] = new LmMap(n->lookForT->lookFor[copyI]->entry, n->lookForT->lookFor[copyI]->size);
            int localDeleted = 0;
            for(int i = 0; i < n->lookForT->lookFor[copyI]->size; i++) {
                if(newlyReachedT.find(n->lookForT->lookFor[copyI]->containedInLMs[i]) == newlyReachedT.end()) {
                    result->lookForT->lookFor[copyI - deleted]->containedInLMs[i - localDeleted] = n->lookForT->lookFor[copyI]->containedInLMs[i];
                } else {
                    //cout << "deleted " << n->lookForT->lookFor[copyI]->containedInLMs[i] << endl;
                    localDeleted++;
                    result->lookForT->lookFor[copyI - deleted]->size--;
                }
            }
            if(result->lookForT->lookFor[copyI - deleted]->size == 0) {
                //cout << "delete task from lookup table" << endl;
                delete result->lookForT->lookFor[copyI - deleted];
                result->lookForT->size--;
                deleted++;
            }

            copyI++;
            ++it;
            if(it == needToModify.end()) {
                //cout << "copy rest" << endl;
                // copy rest
                while(copyI < n->lookForT->size) {
                    //cout << "copy " << copyI << " to index " << (copyI - deleted) << endl;
                    result->lookForT->lookFor[copyI - deleted] = n->lookForT->lookFor[copyI];
                    copyI++;
                }
            }
        }
        //result->lookForT->printTab();
        //cout << "Size: " << result->lookForT->size << endl;
        //exit(19);
    } else {
        result->lookForT = n->lookForT;
        result->lookForT->refCounter++;
    }
#endif

#ifdef TRACKLMS
    if(iu.containsInt(n->tLMs, 0, n->numtLMs - 1, decomposed->task)){
        result->tLMs = iu.copyExcluding(n->tLMs, n->numtLMs, decomposed->task);
        result->numtLMs = n->numtLMs - 1;
        result->reachedtLMs = n->reachedtLMs + 1;
    } else {
        result->tLMs = n->tLMs;
        result->numtLMs = n->numtLMs;
        result->reachedtLMs = n->reachedtLMs;
    }
    if(iu.containsInt(n->mLMs, 0, n->nummLMs - 1, method)){
        result->mLMs = iu.copyExcluding(n->mLMs, n->nummLMs, method);
        result->nummLMs = n->nummLMs - 1;
        result->reachedmLMs = n->reachedmLMs + 1;
    } else {
        result->mLMs = n->mLMs;
        result->nummLMs = n->nummLMs;
        result->reachedmLMs = n->reachedmLMs;
    }
    result->fLMs = n->fLMs;
    result->numfLMs = n->numfLMs;
    result->reachedfLMs = n->reachedfLMs;

    assert(result->numfLMs >= 0);
    assert(result->numtLMs >= 0);
    assert(result->nummLMs >= 0);

    assert(result->numfLMs <= n->numfLMs);
    assert(result->numtLMs <= n->numtLMs);
    assert(result->nummLMs <= n->nummLMs);
#endif

#ifdef PRGEFFECTLESS
    for (int ac = effectLess->getFirst(); ac >= 0; ac = effectLess->getNext()) {
        assert(
                ((this->numAdds[result->unconstraintPrimitive[ac]->task] == 0) && (this->numDels[result->unconstraintPrimitive[ac]->task] == 0) && (this->isApplicable(result, result->unconstraintPrimitive[ac]->task))));
        searchNode *n2 = this->apply(result, ac);
        delete result;
        result = n2;
        numEffLessProg++;
    }
#endif
#ifdef ONEMODAC
    if ((result->numAbstract == 0) && (result->numPrimitive > 0)) {
        int applicable = NOACTION;
        for (int i = 0; i < result->numPrimitive; i++) {
            if (isApplicable(result, result->unconstraintPrimitive[i]->task)) {
                if (applicable == NOACTION) {
                    applicable = i;
                } else {
                    applicable = FORBIDDEN;
                    break;
                }
            }
        }
        if ((applicable != NOACTION) && (applicable != FORBIDDEN)) {
            assert(
                    isApplicable(result, result->unconstraintPrimitive[applicable]->task));
            searchNode *n2 = this->apply(result, applicable);
            delete result;
            result = n2;
            numOneModActions++;
        } else if (applicable == NOACTION) { // there is no abstract task and no applicable primitive
            result->goalReachable = false;
        }
    }
#endif

#ifdef ONEMODMETH
    for (int t = oneMod->getFirst(); t >= 0; t = oneMod->getNext()) {
        assert(
                this->numMethodsForTask[result->unconstraintAbstract[t]->task] == 1);
        searchNode *n2 = this->decompose(result, t, this->taskToMethods[t][0]);
        delete result;
        result = n2;
        numOneModMethods++;
    }
#endif
    return result;
}

pair<planStep**, planStep**> Model::initializeMethod(int method
#ifdef TRACESOLUTION
        , int parentSolutionStepIndex
#endif
        ) {
    planStep** stepPointerList = new planStep*[numSubTasks[method]];
    for (int i = 0; i < numSubTasks[method]; i++) {
        stepPointerList[i] = new planStep;
        stepPointerList[i]->id = ++this->psID;
        stepPointerList[i]->task = subTasks[method][i];
        stepPointerList[i]->pointersToMe = 0;
#ifdef TRACESOLUTION
        stepPointerList[i]->parentSolutionStepInstanceNumber = parentSolutionStepIndex;
        stepPointerList[i]->myPositionInParent = i;
#endif
    }
    for (int i = 0; i < numSubTasks[method]; i++) {
        stepPointerList[i]->numSuccessors = 0;
        if (methodSubtaskSuccNum[method][i] > 0) {
            stepPointerList[i]->successorList =
                    new planStep*[methodSubtaskSuccNum[method][i]];
        }
    }
    for (int i = 0; i < numOrderings[method]; i += 2) {
        int pred = ordering[method][i];
        int succ = ordering[method][i + 1];
        stepPointerList[pred]->successorList[stepPointerList[pred]->numSuccessors++] =
                stepPointerList[succ];
        stepPointerList[succ]->pointersToMe++;
    }

    planStep** firsts = new planStep*[numFirstTasks[method]];
    planStep** lasts = new planStep*[numLastTasks[method]];
    for (int i = 0; i < numFirstTasks[method]; i++) {
        firsts[i] = stepPointerList[methodsFirstTasks[method][i]];
    }
    for (int i = 0; i < numLastTasks[method]; i++) {
        lasts[i] = stepPointerList[methodsLastTasks[method][i]];
    }
    delete[] stepPointerList;
    return make_pair(firsts, lasts);
}

searchNode* Model::apply(searchNode* n, int taskNo) {
    searchNode* result = new searchNode;

    // maintain state
    planStep *progressed = n->unconstraintPrimitive[taskNo];
    assert(isPrimitive[progressed->task]);
#if STATEREP == SRCOPY
    result->state = n->state;

    for (int i = 0; i < numDels[progressed->task]; i++) {
        result->state[delLists[progressed->task][i]] = false;
    }
    for (int i = 0; i < numAdds[progressed->task]; i++) {
        result->state[addLists[progressed->task][i]] = true;
    }
#elif STATEREP == SRLIST
    result->state = new int[n->stateSize + numAdds[progressed->task]];
    int iNew = 0;
    int iOld = 0;
    int iAdd = 0;
    int iDel = 0;
    while (true) {
        if (iOld >= n->stateSize && iAdd >= numAdds[progressed->task]) {
            result->stateSize = iNew;
            break;
        } else if (iOld >= n->stateSize && iAdd < numAdds[progressed->task]) {
            result->state[iNew] = addLists[progressed->task][iAdd]; // here, del is irrelevant
            iNew++;
            iAdd++;
        } else if ((iOld < n->stateSize && iAdd >= numAdds[progressed->task])
                || (addLists[progressed->task][iAdd] > n->state[iOld])) {
            while ((iDel < numDels[progressed->task])
                    && (delLists[progressed->task][iDel] < n->state[iOld])) {
                iDel++;
            }
            if ((iDel >= numDels[progressed->task])
                    || (delLists[progressed->task][iDel] != n->state[iOld])) {
                result->state[iNew] = n->state[iOld];
                iNew++;
            } // else, this bit has been deleted by the action
            iOld++;
        } else if (addLists[progressed->task][iAdd] < n->state[iOld]) {
            // same case as above, but now I know that the indices are small enough
            result->state[iNew] = addLists[progressed->task][iAdd];// here, del is irrelevant
            iNew++;
            iAdd++;
        } else { // old state bit and add are equal
            iOld++;
        }
    }
#endif
    assert(isApplicable(n, progressed->task));
    // every successor of ps is a first task if and only if it is
    // not a successor of any task in the firstTasks list.
    for (int i = 0; i < progressed->numSuccessors; i++){
        potentiallyFirst.insert(progressed->successorList[i]);
        for (int j = 0; j < progressed->successorList[i]->numSuccessors; j++) {
            potentialPredecessors.push_front(
                    progressed->successorList[i]->successorList[j]);
        }
        //cout << "PF: " << progressed->successorList[i] << endl;
    }

    for (int i = 0; i < n->numAbstract; i++) {
        for (int j = 0; j < n->unconstraintAbstract[i]->numSuccessors; j++) {
            potentialPredecessors.push_front(
                    n->unconstraintAbstract[i]->successorList[j]);
        }
        //cout << "PP: " << n->unconstraintAbstract[i] << endl;
    }
    for (int i = 0; i < n->numPrimitive; i++) {
        if (i != taskNo) {
            for (int j = 0; j < n->unconstraintPrimitive[i]->numSuccessors;
                    j++) {
                potentialPredecessors.push_front(
                        n->unconstraintPrimitive[i]->successorList[j]);
            }
            //cout << "PP: " << n->unconstraintPrimitive[i] << endl;
        }

    }
    while (true) {
        if (potentiallyFirst.empty()) {
            break; // no first left
        }
        if (potentialPredecessors.empty()) {
            break; // the next that are left are valid firstTasks
        }
        planStep* ps2 = potentialPredecessors.front();
        potentialPredecessors.pop_front();
        done.insert(ps2);
        //cout << "GET " << ps2 << endl;
        set<planStep*>::iterator iter = potentiallyFirst.find(ps2);
        if (iter != potentiallyFirst.end()) {
            potentiallyFirst.erase(iter);
            //cout << "ERASE " << ps2 << endl;
        }

        for (int i = 0; i < ps2->numSuccessors; i++) {
            planStep* ps = ps2->successorList[i];
            if (done.find(ps) == done.end()) {
                potentialPredecessors.push_front(ps);
            }
        }
    }

    // there may be more, but these are the basic vals:
    result->numPrimitive = n->numPrimitive - 1; // one has been progressed
    result->numAbstract = n->numAbstract;

    // add positions for tasks that are successors of the progressed one
    for (planStep* ps2 : potentiallyFirst) {
        if (isPrimitive[ps2->task]) {
            result->numPrimitive++;
        } else {
            result->numAbstract++;
        }
    }
    result->unconstraintAbstract = new planStep*[result->numAbstract];
    result->unconstraintPrimitive = new planStep*[result->numPrimitive];
#ifdef ONEMODMETH
    oneMod->clear();
#endif

    int currentA = 0;
    int currentP = 0;
    for (int i = 0; i < n->numAbstract; i++) {
        result->unconstraintAbstract[currentA] = n->unconstraintAbstract[i];
#ifdef ONEMODMETH
        if (numMethodsForTask[result->unconstraintAbstract[currentA]->task]
                == 1)
            oneMod->push(currentA);
#endif
        currentA++;
    }
#ifdef PRGEFFECTLESS
    effectLess->clear();
#endif
    for (int i = 0; i < n->numPrimitive; i++) {
        if (i != taskNo) {
            result->unconstraintPrimitive[currentP] =
                    n->unconstraintPrimitive[i];
#ifdef PRGEFFECTLESS
            if ((this->numAdds[result->unconstraintPrimitive[currentP]->task]
                    == 0)
                    && (this->numDels[result->unconstraintPrimitive[currentP]->task]
                            == 0)
                    && (this->isApplicable(result,
                            result->unconstraintPrimitive[currentP]->task))) {
                effectLess->push(currentP);
            }
#endif
            currentP++;
        }
    }

    for (planStep* ps2 : potentiallyFirst) {
        if (isPrimitive[ps2->task]) {
            result->unconstraintPrimitive[currentP] = ps2;
#ifdef PRGEFFECTLESS
            if ((this->numAdds[result->unconstraintPrimitive[currentP]->task]
                    == 0)
                    && (this->numDels[result->unconstraintPrimitive[currentP]->task]
                            == 0)
                    && (this->isApplicable(result,
                            result->unconstraintPrimitive[currentP]->task))) {
                effectLess->push(currentP);
            }
#endif
            currentP++;
        } else {
            result->unconstraintAbstract[currentA] = ps2;
#ifdef ONEMODMETH
            if (numMethodsForTask[result->unconstraintAbstract[currentA]->task]
                    == 1)
                oneMod->push(currentA);
#endif
            currentA++;
        }
    }

    potentiallyFirst.clear();
    done.clear();
    potentialPredecessors.clear();

    result->solution = new solutionStep();
    result->solution->task = progressed->task;
    result->solution->method = -1;
    result->solution->prev = n->solution;
#ifdef TRACESOLUTION
    result->solution->mySolutionStepInstanceNumber = progression::currentSolutionStepInstanceNumber++;
    result->solution->myPositionInParent = progressed->myPositionInParent;
    result->solution->parentSolutionStepInstanceNumber = progressed->parentSolutionStepInstanceNumber;
#endif
    result->solution->pointersToMe = 1;
    result->modificationDepth = n->modificationDepth + 1;
    result->mixedModificationDepth = n->mixedModificationDepth + this->actionCosts[progressed->task];
    result->actionCosts = n->actionCosts + this->actionCosts[progressed->task];

    // maintain pointer counter
    n->solution->pointersToMe++;

    for (int i = 0; i < result->numPrimitive; i++) {
        result->unconstraintPrimitive[i]->pointersToMe++;
    }
    for (int i = 0; i < result->numAbstract; i++) {
        result->unconstraintAbstract[i]->pointersToMe++;
    }
#ifdef MAINTAINREACHABILITY
    updateReachability(result);
#endif

#ifdef TRACKTASKSINTN
    updateTaskCounterA(result, n, n->unconstraintPrimitive[taskNo]->task);
#endif

#ifdef TRACKLMSFULL
    //cout << "action application" << endl;
    result->lms = n->lms;
    result->numLMs = n->numLMs;
    result->lookForT = n->lookForT;
    result->lookForT->refCounter++;
    result->lookForM = n->lookForM;
    result->lookForM->refCounter++;

    set<int> newlyReachedLM;
    for (int i = 0; i < numAdds[progressed->task]; i++) {
        int addedF = addLists[progressed->task][i];
        if(!n->state[addedF]) {
             // this effect is newly set to true
            int iLmMapping = n->lookForF->indexOf(addedF);
            if (iLmMapping >= 0) { // check LMs for this task
                for(int iLMM = 0; iLMM < n->lookForF->lookFor[iLmMapping]->size; iLMM++){
                    int iLMglobal = n->lookForF->lookFor[iLmMapping]->containedInLMs[iLMM];
                    if(newlyReachedLM.find(iLMglobal) != newlyReachedLM.end())
                        continue;
                    landmark* lm = n->lms[iLMglobal];
                    if((lm->connection == atom) || (lm->connection == disjunctive)) {
                        newlyReachedLM.insert(iLMglobal);
                    } else if(lm->connection == conjunctive) {
                        // more than one state feature contained in the lm might be set by the current action
                        // therefore we use the "result" object for the following check
                        bool fulfilled = true;
                        for(int iCon = 0; iCon < lm->size; iCon++) {
                            int fCon = lm->lm[iCon];
                            if(!result->state[fCon]){
                                fulfilled = false;
                                break;
                            }
                        }
                        if(fulfilled){
                            newlyReachedLM.insert(iLMglobal);
                        }
                    }
                }
            }
        }
    }
    if(newlyReachedLM.size() > 0) {
        //n->lookForF->printTab();

        /*cout << "effects ";
        for (int i = 0; i < numAdds[progressed->task]; i++) {
            cout << addLists[progressed->task][i] << " ";
        }
        cout << endl;*/
        // copy lm data, delete reached lms from "lookFor" lists
        set<int> needToModify; // these are the tasks that need to be modified
        for(int lmID : newlyReachedLM) {
            landmark* lm = n->lms[lmID];
            //cout << lmID << " ";
            //lm->printLM();
            for(int iT = 0; iT < lm->size; iT++) {
                needToModify.insert(lm->lm[iT]);
            }
        }

        /*
        cout << "Need to modify ";
        for(int e : needToModify) {
            cout << e << " ";
        }
        cout << endl;*/

        set<int>::iterator it = needToModify.begin();
        int copyI = 0;
        int deleted = 0;
        result->lookForF = new lookUpTab(n->lookForF->size);
        while (it != needToModify.end()) {
            int taskToModify = (*it);
            //cout << "taskToModify " << taskToModify << endl;
            // copy until finding a task with a mapping that needs to be modified
            while(n->lookForF->lookFor[copyI]->entry < taskToModify) {
                result->lookForF->lookFor[copyI - deleted] = n->lookForF->lookFor[copyI];
                //cout << "copy " << n->lookForF->lookFor[copyI]->entry << endl;
                copyI++;
            }
            assert(taskToModify == n->lookForF->lookFor[copyI]->entry);

            // modify the entry
            result->lookForF->lookFor[copyI - deleted] = new LmMap(n->lookForF->lookFor[copyI]->entry, n->lookForF->lookFor[copyI]->size);
            int localDeleted = 0;
            for(int i = 0; i < n->lookForF->lookFor[copyI]->size; i++) {
                int lm = n->lookForF->lookFor[copyI]->containedInLMs[i];
                if(newlyReachedLM.find(lm) == newlyReachedLM.end()) {
                    result->lookForF->lookFor[copyI - deleted]->containedInLMs[i - localDeleted] = n->lookForF->lookFor[copyI]->containedInLMs[i];
                } else {
                    //cout << "deleted " << n->lookForF->lookFor[copyI]->containedInLMs[i] << endl;
                    localDeleted++;
                    result->lookForF->lookFor[copyI - deleted]->size--;
                }
            }
            if(result->lookForF->lookFor[copyI - deleted]->size == 0) {
                //cout << "delete task from lookup table" << endl;
                delete result->lookForF->lookFor[copyI - deleted];
                result->lookForF->size--;
                deleted++;
            }

            copyI++;
            ++it;
            if(it == needToModify.end()) {
                //cout << "copy rest" << endl;
                // copy rest
                while(copyI < n->lookForF->size) {
                    //cout << "copy " << copyI << " to index " << (copyI - deleted) << endl;
                    result->lookForF->lookFor[copyI - deleted] = n->lookForF->lookFor[copyI];
                    copyI++;
                }
            }
        }
        //result->lookForF->printTab();
        //cout << "Size: " << result->lookForF->size << endl;

        //exit(19);
    } else {
        result->lookForF = n->lookForF;
        result->lookForF->refCounter++;
    }
#endif

#ifdef TRACKLMS
    if(iu.containsInt(n->tLMs, 0, n->numtLMs - 1, progressed->task)){
        result->tLMs = iu.copyExcluding(n->tLMs, n->numtLMs, progressed->task);
        result->numtLMs = n->numtLMs - 1;
        result->reachedtLMs = n->reachedtLMs + 1;
    } else {
        result->tLMs = n->tLMs;
        result->numtLMs = n->numtLMs;
        result->reachedtLMs = n->reachedtLMs;
    }
    bool containsOne = false; // at least (!) one
    int iEff = 0;
    while((!containsOne) && (iEff < this->numAdds[progressed->task])){
        containsOne = iu.containsInt(n->fLMs,0,n->numfLMs-1,this->addLists[progressed->task][iEff]);
        iEff++;
    }
    if (containsOne){
        result->fLMs = new int[n->numfLMs - 1]; // might be too large, but not too small
        result->numfLMs = 0;
        result->reachedfLMs = n->reachedfLMs;
        for(int iLM = 0; iLM < n->numfLMs; iLM++) {
            int lm = n->fLMs[iLM];
            if (!iu.containsInt(this->addLists[progressed->task], 0, this->numAdds[progressed->task] - 1, lm)){
                result->fLMs[result->numfLMs++] = lm;
            } else {
                result->reachedfLMs++;
            }
        }
    } else {
        result->fLMs = n->fLMs;
        result->numfLMs = n->numfLMs;
        result->reachedfLMs = n->reachedfLMs;
    }
    result->mLMs = n->mLMs;
    result->nummLMs = n->nummLMs;
    result->reachedmLMs = n->reachedmLMs;
#endif

#ifdef PRGEFFECTLESS
    for (int ac = effectLess->getFirst(); ac >= 0; ac = effectLess->getNext()) {
        assert(
                ((this->numAdds[result->unconstraintPrimitive[ac]->task] == 0) && (this->numDels[result->unconstraintPrimitive[ac]->task] == 0) && (this->isApplicable(result, result->unconstraintPrimitive[ac]->task))));
        searchNode *n2 = this->apply(result, ac);
        delete result;
        result = n2;
        numEffLessProg++;
    }
#endif
#ifdef ONEMODAC
    if ((result->numAbstract == 0) && (result->numPrimitive > 0)) {
        int applicable = NOACTION;
        for (int i = 0; i < result->numPrimitive; i++) {
            if (isApplicable(result, result->unconstraintPrimitive[i]->task)) {
                if (applicable == NOACTION) {
                    applicable = i;
                } else {
                    applicable = FORBIDDEN;
                    break;
                }
            }
        }
        if ((applicable != NOACTION) && (applicable != FORBIDDEN)) {
            assert(
                    isApplicable(result, result->unconstraintPrimitive[applicable]->task));
            searchNode *n2 = this->apply(result, applicable);
            delete result;
            result = n2;
            numOneModActions++;
        } else if (applicable == NOACTION) { // there is no abstract task and no applicable primitive
            result->goalReachable = false;
        }
    }
#endif

#ifdef ONEMODMETH
    for (int t = oneMod->getFirst(); t >= 0; t = oneMod->getNext()) {
        assert(
                this->numMethodsForTask[result->unconstraintAbstract[t]->task] == 1);
        searchNode *n2 = this->decompose(result, t, this->taskToMethods[t][0]);
        delete result;
        result = n2;
        numOneModMethods++;
    }
#endif
    return result;
}

#if STATEREP == SRCOPY

bool Model::isApplicable(searchNode *n, int action) const {
    for (int i = 0; i < numPrecs[action]; i++) {
        if (!n->state[precLists[action][i]])
            return false;
    }
    return true;
}

bool Model::isGoal(searchNode *n) const {
    if ((n->numAbstract > 0) || (n->numPrimitive > 0))
        return false;
    for (int i = 0; i < gSize; i++) {
        if (!n->state[gList[i]])
            return false;
    }
    return true;
}

#elif STATEREP == SRLIST

bool Model::isApplicable(searchNode *n, int action) const {
    int precI = 0;
    int stateI = 0;
    while (true) {
        if (precI == numPrecs[action])
        return true;
        if (stateI == n->stateSize)
        return false;
        if (precLists[action][precI] > n->state[stateI]) {
            stateI++;
        } else if (precLists[action][precI] == n->state[stateI]) {
            precI++;
            stateI++;
        } else { // i.e. precLists[action][precI] < n->state[stateI]
            return false;
        }
    }
    return true;
}

bool Model::isGoal(searchNode *n) const {
    if ((n->numAbstract > 0) || (n->numPrimitive > 0))
    return false;
    int gI = 0;
    int stateI = 0;
    while (gI < gSize) {
        if (gList[gI] > n->state[stateI]) {
            stateI++;
        } else if (gList[gI] == n->state[stateI]) {
            gI++;
            stateI++;
        } else { // i.e. gList[precI] < n->state[stateI]
            return false;
        }
    }
    return true;
}

#elif (STATEREP == SRCALC1) || (STATEREP == SRCALC2)

bool Model::stateFeatureHolds(int f, searchNode* n) const {
    solutionStep* sol = n->solution;
    while (sol->prev != nullptr) {
        if (isPrimitive[sol->task]) {
            if (addVectors[sol->task][f])
            return true;
            if (delVectors[sol->task][f])
            return false;
        }
        sol = sol->prev;
    }
    return s0Vector[f];
}

bool Model::isApplicable(searchNode *n, int action) const {
    for (int i = 0; i < numPrecs[action]; i++) {
        if (!stateFeatureHolds(precLists[action][i], n))
        return false;
    }
    return true;
}

bool Model::isGoal(searchNode *n) const {
    if ((n->numAbstract > 0) || (n->numPrimitive > 0))
    return false;
    for (int i = 0; i < gSize; i++) {
        if (!stateFeatureHolds(gList[i], n))
        return false;
    }
    return true;
}

#endif

#ifdef MAINTAINREACHABILITY
void Model::updateReachability(searchNode *n) {
    for (int i = 0; i < n->numAbstract; i++) {
        if (n->unconstraintAbstract[i]->reachableT == nullptr) {
            calcReachability(n->unconstraintAbstract[i]);
        }
    }
    for (int i = 0; i < n->numPrimitive; i++) {
        if (n->unconstraintPrimitive[i]->reachableT == nullptr) {
            calcReachability(n->unconstraintPrimitive[i]);
        }
    }
}

void Model::calcReachability(planStep *ps) {
    // call for subtasks
    for (int i = 0; i < ps->numSuccessors; i++) {
        if (ps->successorList[i]->reachableT == nullptr) {
            calcReachability(ps->successorList[i]);
        }
    }

    // calc reachability for this step
    intSet.clear();
    for (int i = 0; i < ps->numSuccessors; i++) {
        for (int j = 0; j < ps->successorList[i]->numReachableT; j++) {
            intSet.insert(ps->successorList[i]->reachableT[j]);
        }
    }
    for (int i = 0; i < this->numReachable[ps->task]; i++) {
        intSet.insert(this->reachable[ps->task][i]);
    }
    ps->numReachableT = intSet.getSize();
    ps->reachableT = new int[ps->numReachableT];
    int k = 0;
    for (int t = intSet.getFirst(); t >= 0; t = intSet.getNext()) {
        ps->reachableT[k++] = t;
    }
}
#endif

void Model::generateMethodRepresentation() {

    // generate mapping from a task to all methods applicable to it
    vector<int>* tTaskToMethods = new vector<int> [numTasks];

    for (int i = 0; i < numMethods; i++) {
        tTaskToMethods[decomposedTask[i]].push_back(i);
    }

    taskToMethods = new int*[numTasks];
    numMethodsForTask = new int[numTasks];
    for (int i = 0; i < numTasks; i++) {
        numMethodsForTask[i] = tTaskToMethods[i].size();
        if (numMethodsForTask[i] > 0) {
            assert(!isPrimitive[i]);
            taskToMethods[i] = new int[numMethodsForTask[i]];
        } else {
            taskToMethods[i] = nullptr;
            if (!isPrimitive[i]) {
                cout << "Warning: The task " << taskNames[i]
                        << " is abstract but there is no method to decompose it."
                        << endl;
            }
        }
        for (int j = 0; j < numMethodsForTask[i]; j++) {
            taskToMethods[i][j] = tTaskToMethods[i][j];
        }
    }
    delete[] tTaskToMethods;

    // generate structures needed to instantiate methods
    methodsFirstTasks = new int*[numMethods];
    numFirstTasks = new int[numMethods];
    numFirstAbstractSubTasks = new int[numMethods];
    numFirstPrimSubTasks = new int[numMethods];
    methodsLastTasks = new int*[numMethods];
    numLastTasks = new int[numMethods];
    methodSubtaskSuccNum = new int*[numMethods];

    for (int i = 0; i < numMethods; i++) {
        bool * firsts = new bool[numSubTasks[i]];
        bool * lasts = new bool[numSubTasks[i]];
        methodSubtaskSuccNum[i] = new int[numSubTasks[i]];
        for (int j = 0; j < numSubTasks[i]; j++) {
            methodSubtaskSuccNum[i][j] = 0;
        }
        for (int ps = 0; ps < numSubTasks[i]; ps++) {
            firsts[ps] = true;
            lasts[ps] = true;
        }
        for (int o = 0; o < numOrderings[i]; o += 2) {
            int pred = ordering[i][o];
            int succ = ordering[i][o + 1];
            methodSubtaskSuccNum[i][pred]++;
            firsts[succ] = false;
            lasts[pred] = false;
        }
        numFirstTasks[i] = 0;
        numFirstPrimSubTasks[i] = 0;
        numFirstAbstractSubTasks[i] = 0;
        numLastTasks[i] = 0;
        for (int j = 0; j < numSubTasks[i]; j++) {
            if (firsts[j]) {
                numFirstTasks[i]++;
                if (isPrimitive[subTasks[i][j]]) {
                    numFirstPrimSubTasks[i]++;
                } else {
                    numFirstAbstractSubTasks[i]++;
                }
            }
            if (lasts[j])
                numLastTasks[i]++;
        }
        methodsFirstTasks[i] = new int[numFirstTasks[i]];
        methodsLastTasks[i] = new int[numLastTasks[i]];
        int curFI = 0;
        int curLI = 0;
        for (int j = 0; j < numSubTasks[i]; j++) {
            if (firsts[j])
                methodsFirstTasks[i][curFI++] = j;
            if (lasts[j])
                methodsLastTasks[i][curLI++] = j;
        }
        assert(curFI == numFirstTasks[i]);
        assert(curLI == numLastTasks[i]);
        delete[] firsts;
        delete[] lasts;
    }
}

void Model::readClassical(istream& domainFile) {
    string line;
    getline(domainFile, line);
    stringstream* sStream;
    sStream = new stringstream(line);
    // read state bits and their descriptions
    *sStream >> numStateBits;
    delete sStream;
    factStrs = new string[numStateBits];
    for (int i = 0; i < numStateBits; i++) {
        getline(domainFile, line);
        factStrs[i] = line;
    }
    getline(domainFile, line);
    getline(domainFile, line);
    // variable definitions
    getline(domainFile, line);
    sStream = new stringstream(line);
    *sStream >> numVars;
    delete sStream;
    firstIndex = new int[numVars];
    lastIndex = new int[numVars];
    varNames = new string[numVars];
    for (int i = 0; i < numVars; i++) {
        getline(domainFile, line);
        sStream = new stringstream(line);
        *sStream >> firstIndex[i];
        assert(firstIndex[i] < numStateBits);
        *sStream >> lastIndex[i];
        assert(lastIndex[i] < numStateBits);
        *sStream >> varNames[i];
        delete sStream;
    }
    getline(domainFile, line);
    getline(domainFile, line);

    for (int informationType = 0; informationType < 3; informationType++){
        int & num = (informationType == 0) ? numStrictMutexes : ((informationType == 1) ? numMutexes : numInvariants);
        int* & size = (informationType == 0) ? strictMutexesSize : ((informationType == 1) ? mutexesSize : invariantsSize);
        int** & elems = (informationType == 0) ? strictMutexes : ((informationType == 1) ? mutexes : invariants);

        // read further information
        getline(domainFile, line);
        sStream = new stringstream(line);
        *sStream >> num;
        delete sStream;
        elems = new int*[num];
        size = new int[num];
        for (int i = 0; i < num; i++){
            getline(domainFile, line);
            elems[i] = readIntList(line, size[i]);
        }
        getline(domainFile, line);
        getline(domainFile, line);
    }


    // read actions
    getline(domainFile, line);
    sStream = new stringstream(line);
    *sStream >> numActions;
    delete sStream;
    actionCosts = new int[numActions];

    numPrecs = new int[numActions];
    precLists = new int*[numActions];

    numAdds = new int[numActions];
    numDels = new int[numActions];
    addLists = new int*[numActions];
    delLists = new int*[numActions];

    numConditionalAdds = new int[numActions];
    numConditionalAddsConditions = new int*[numActions];
    numConditionalDels = new int[numActions];
    numConditionalDelsConditions = new int*[numActions];
    conditionalDelLists = new int*[numActions];
    conditionalDelListsCondition = new int**[numActions];
    conditionalAddLists = new int*[numActions];
    conditionalAddListsCondition = new int**[numActions];

    numPrecLessActions = 0;
    for (int i = 0; i < numActions; i++) {
        getline(domainFile, line);
        sStream = new stringstream(line);
        *sStream >> actionCosts[i];
        delete sStream;
        getline(domainFile, line);
        precLists[i] = readIntList(line, numPrecs[i]);
        if (numPrecs[i] == 0) {
            numPrecLessActions++;
        }

        getline(domainFile, line);
        std::tuple<int*,int*,int**> adds = readConditionalIntList(line, numAdds[i], numConditionalAdds[i], numConditionalAddsConditions[i]);
        addLists[i] = get<0>(adds);
        conditionalAddLists[i] = get<1>(adds);
        conditionalAddListsCondition[i] = get<2>(adds);


        getline(domainFile, line);
        std::tuple<int*,int*,int**> dels = readConditionalIntList(line, numDels[i], numConditionalDels[i], numConditionalDelsConditions[i]);
        delLists[i] = get<0>(dels);
        conditionalDelLists[i] = get<1>(dels);
        conditionalDelListsCondition[i] = get<2>(dels);

#ifndef NDEBUG
        for(int j = 0; j < numPrecs[i]; j++){
            assert(precLists[i][j] < numStateBits);
        }
        for(int j = 0; j < numAdds[i]; j++){
            assert(addLists[i][j] < numStateBits);
        }
        for(int j = 0; j < numDels[i]; j++){
            assert(delLists[i][j] < numStateBits);
        }
#endif
    }

    // determine set of actions that have no precondition
    if (numPrecLessActions > 0) {
        int cur = 0;
        precLessActions = new int[numPrecLessActions];
        for (int i = 0; i < numActions; i++) {
            if (numPrecs[i] == 0) {
                precLessActions[cur++] = i;
            }
        }
        assert(cur == numPrecLessActions);
    } else {
        precLessActions = nullptr;
    }

    for (int i = 0; i < numActions; i++) {
        set<int> intSet;
        for (int j = 0; j < numPrecs[i]; j++) {
            assert(precLists[i][j] < numStateBits);
            intSet.insert(precLists[i][j]);
        }
        int intSize = intSet.size();
        if (intSize < numPrecs[i]) {
            cout
                    << "Action #" << i << " prec/add/del-definition contains same state feature twice"
                    << endl;
#ifndef NDEBUG
            cout << "Original:";
            for (int j = 0; j < numPrecs[i]; j++) cout << " " << precLists[i][j];
            cout << endl;
#endif
            numPrecs[i] = intSet.size();
            delete[] precLists[i];
            precLists[i] = new int[numPrecs[i]];
            int cur = 0;
            for (int p : intSet) {
                precLists[i][cur++] = p;
            }
            assert(cur == intSize);
        }
    }

    set<int> * precToActionTemp = new set<int>[numStateBits];
    for (int i = 0; i < numActions; i++) {
        for (int j = 0; j < numPrecs[i]; j++) {
            int f = precLists[i][j];
            precToActionTemp[f].insert(i);
        }
    }
    precToActionSize = new int[numStateBits];
    precToAction = new int*[numStateBits];

    for (int i = 0; i < numStateBits; i++) {
        precToActionSize[i] = precToActionTemp[i].size();
        precToAction[i] = new int[precToActionSize[i]];
        int cur = 0;
        for (int ac : precToActionTemp[i]) {
            precToAction[i][cur++] = ac;
        }
    }
    delete[] precToActionTemp;
    // s0
    getline(domainFile, line);
    getline(domainFile, line);
    getline(domainFile, line);
    s0List = readIntList(line, s0Size);
    // goal
    getline(domainFile, line);
    getline(domainFile, line);
    getline(domainFile, line);
    gList = readIntList(line, gSize);
#ifndef NDEBUG
        for(int j = 0; j < s0Size; j++){
            assert(s0List[j] < numStateBits);
        }
        for(int j = 0; j < gSize; j++){
            assert(gList[j] < numStateBits);
        }
#endif

    // task/action names
    getline(domainFile, line);
    getline(domainFile, line);
    getline(domainFile, line);
    isHtnModel = true;
    sStream = new stringstream(line);
    *sStream >> numTasks;
    delete sStream;
    taskNames = new string[numTasks];
    isPrimitive = new bool[numTasks];
    bool isAbstract;
    for (int i = 0; i < numTasks; i++) {
        getline(domainFile, line);
        if (line.size() == 0){
            cout << "Input promised " << numTasks << " tasks, but the list ended after " << i << endl;
            exit(-1);
        }
        sStream = new stringstream(line);
        *sStream >> isAbstract;
        isPrimitive[i] = !isAbstract;
        *sStream >> taskNames[i];
        delete sStream;
    }
}

void Model::readHierarchical(istream& domainFile) {
    stringstream* sStream;
    string line;
    // tasks
    for (int i = 0; i < 3; i++) {
        getline(domainFile, line);
        if (!i && line.size()){
            cout << "Excess task (list of tasks is longer than expected)" << endl;
            exit(-1);
        }
        if (domainFile.eof()) {
            isHtnModel = false;
            return;
        }
    }
    isHtnModel = true;
    sStream = new stringstream(line);
    *sStream >> initialTask;
    delete sStream;
    assert(initialTask < numTasks);
    // methods
    getline(domainFile, line);
    getline(domainFile, line);
    getline(domainFile, line);
    sStream = new stringstream(line);
    *sStream >> numMethods;
    delete sStream;
    decomposedTask = new int[numMethods];
    subTasks = new int*[numMethods];
    numSubTasks = new int[numMethods];
    ordering = new int*[numMethods];
    numOrderings = new int[numMethods];
    methodNames = new string[numMethods];
    for (int i = 0; i < numMethods; i++) {
        getline(domainFile, line);
        if (domainFile.eof()){
            cout << "Input promised " << numMethods << " methods, but input ended after " << i << endl;
            exit(-1);
        }
        methodNames[i] = line;
        getline(domainFile, line);
        sStream = new stringstream(line);
        *sStream >> decomposedTask[i];
        assert(decomposedTask[i] < numTasks);
        delete sStream;
        getline(domainFile, line);
        subTasks[i] = readIntList(line, numSubTasks[i]);
        if (numSubTasks[i] == 0) {
            cout << "Search engine: Method " << methodNames[i]
                    << " has no subtasks - please compile this away before search."
                    << endl;
            exit(-1);
        }
#ifndef NDEBUG
        for (int j = 0; j < numSubTasks[i]; j++) {
            assert(subTasks[i][j] < numTasks);
        }
#endif
        getline(domainFile, line);
        ordering[i] = readIntList(line, numOrderings[i]);


        //vector<vector<int>> adj (numSubTasks[i]);
        //for (int o = 0; o < numOrderings[i]; o+=2)
        //  adj[ordering[i][o]].push_back(ordering[i][o+1]);

        // transitive reduction (i.e. remove all unnecessary edges)
        vector<vector<bool>> trans (numSubTasks[i]);
        for (int x = 0; x < numSubTasks[i]; x++)
            for (int y = 0; y < numSubTasks[i]; y++) trans[x].push_back(false);

        for (int o = 0; o < numOrderings[i]; o+=2)
            trans[ordering[i][o]][ordering[i][o+1]] = true;

        for (int k = 0; k < numSubTasks[i]; k++)
            for (int x = 0; x < numSubTasks[i]; x++)
                for (int y = 0; y < numSubTasks[i]; y++)
                    if (trans[x][k] && trans[k][y]) trans[x][y] = false;

        vector<int> ord;
        for (int x = 0; x < numSubTasks[i]; x++)
            for (int y = 0; y < numSubTasks[i]; y++)
                if (trans[x][y])
                    ord.push_back(x), ord.push_back(y);

        ordering[i] = new int[ord.size()];
        for (int x = 0; x < ord.size(); x++)
            ordering[i][x] = ord[x];
        numOrderings[i] = ord.size();


#ifndef NDEBUG
        assert((numOrderings[i] % 2) == 0);
        for (int j = 0; j < numOrderings[i]; j++) {
            assert(ordering[i][j] < numSubTasks[i]);
        }

        // test if subtask ordering is cyclic
        set<int> ignore;
        bool changed = true;
        while(changed) {
            changed = false;
            for(int st = 0; st < numSubTasks[i]; st++) {
                if (ignore.find(st) != ignore.end()) continue;

                bool stHasPred = false;
                for(int o = 0; o < numOrderings[i]; o += 2) {
                    int t1 = ordering[i][o];
                    int t2 = ordering[i][o + 1];
                    if (ignore.find(t1) != ignore.end()) continue;
                    if (t2 == st) {
                        stHasPred = true;
                        break;
                    }
                }
                if(stHasPred) continue;
                else {
                    ignore.insert(st);
                    changed = true;
                    break;
                }
            }
        }
        if(ignore.size() < numSubTasks[i]) {
            cout << "Ordering relations of method " << methodNames[i] << " are cyclic.";
            assert(ignore.size() < numSubTasks[i]);
        }
        for(int o = 0; o < numOrderings[i]; o += 2) {
            int t11 = ordering[i][o];
            int t12 = ordering[i][o + 1];
            for(int o2 = o + 2; o2 < numOrderings[i]; o2 += 2) {
                int t21 = ordering[i][o2];
                int t22 = ordering[i][o2 + 1];

                if((t11 == t21) && (t12 == t22)){
                    cout << "Ordering relations of method " << methodNames[i] << " are redundant." << endl;
                    assert(false);
                }
            }
        }
#endif
    }

    // Mapping from task to methods where it is a subtasks
    stToMethodNum = new int[this->numTasks];
    for (int i = 0; i < this->numTasks; i++)
        stToMethodNum[i] = 0;
    for (int iM = 0; iM < this->numMethods; iM++) {
        for (int iST = 0; iST < this->numSubTasks[iM]; iST++) {
            int st = this->subTasks[iM][iST];
            stToMethodNum[st]++;
        }
    }

    stToMethod = new int*[this->numTasks];
    int * k = new int[this->numTasks];
    for (int i = 0; i < this->numTasks; i++) {
        stToMethod[i] = new int[stToMethodNum[i]];
        k[i] = 0;
    }

    for (int m = 0; m < this->numMethods; m++) {
        for (int iST = 0; iST < this->numSubTasks[m]; iST++) {
            int st = this->subTasks[m][iST];
            if(iu.indexOf(stToMethod[st], 0, k[st] - 1, m) < 0) {
                stToMethod[st][k[st]++] = m;
            } else {
                stToMethodNum[st]--;
            }
        }
    }
    delete[] k;
#ifndef NDEBUG
    /*
     * check mapping
     */
    for(int m = 0; m < this->numMethods; m++) {
        for(int iST = 0; iST < this->numSubTasks[m]; iST++) {
            int st = this->subTasks[m][iST];
            int i = iu.indexOf(stToMethod[st], 0, stToMethodNum[st] - 1, m);
            assert(i >= 0);
        }
    }
    set<int> test;
    for(int t = 0; t < numTasks; t++) {
        test.clear();
        for(int iM = 0; iM < stToMethodNum[t]; iM++) {
            int m = stToMethod[t][iM];
            test.insert(m);
        }
        assert(test.size() == stToMethodNum[t]);
    }
    for(int t = 0; t < numTasks; t++) {
        for(int iM = 0; iM < stToMethodNum[t]; iM++) {
            int m = stToMethod[t][iM];
            bool isIn = false;
            for(int iST = 0; iST < this->numSubTasks[m]; iST++) {
                int st = this->subTasks[m][iST];
                if (st == t){
                    isIn = true;
                    break;
                }
            }
            assert(isIn);
        }
    }
#endif

    // sets of distinct subtasks of methods
    this->numDistinctSTs = new int[numMethods];
    this->sortedDistinctSubtasks = new int*[numMethods];
    this->sortedDistinctSubtaskCount = new int*[numMethods];

    for(int m =0;  m < this->numMethods; m++) {
        int n = this->numSubTasks[m];
        int* sts = new int[n];
        int* stcount = new int[n];

        for(int ist = 0; ist < n; ist++) {
            sts[ist] = this->subTasks[m][ist];
            stcount[ist] = 1;
        }

        for(int ist = 0; ist < n; ist++) {
            stcount[ist] = 1;
        }
        iu.sort(sts, 0, n - 1);

        for(int i = 0; i < n - 1; i++) {
            int j = 0;
            while(((i + j + 1) < n) && (sts[i] == sts[i + j + 1])) {
                j++;
            }
            if(j > 0) {
                stcount[i] += j;
                int k = 0;
                while ((i + k + j) < n) {
                    sts[i + k] = sts[i + k + j];
                    k++;
                }
                n -= j;
            }
        }
        this->numDistinctSTs[m] = n;
        this->sortedDistinctSubtasks[m] = sts;
        this->sortedDistinctSubtaskCount[m] = stcount;
    }
}

#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
void Model::generateVectorRepresentation() {
    addVectors = new bool*[numActions];
    delVectors = new bool*[numActions];
    for (int i = 0; i < numActions; i++) {
        addVectors[i] = new bool[numStateBits];
        delVectors[i] = new bool[numStateBits];
        for (int j = 0; j < numStateBits; j++) {
            addVectors[i][j] = false;
            delVectors[i][j] = false;
        }
        for (int j = 0; j < numAdds[i]; j++) {
            addVectors[i][addLists[i][j]] = true;
        }
        for (int j = 0; j < numDels[i]; j++) {
            delVectors[i][delLists[i][j]] = true;
        }
    }
    s0Vector = new bool[numStateBits];
    for (int i = 0; i < numStateBits; i++) {
        s0Vector[i] = false;
    }
    for (int i = 0; i < s0Size; i++) {
        s0Vector[s0List[i]] = true;
    }
}
#endif

void Model::read(string f) {

    std::istream * inputStream;
    if (f == "stdin"){
        inputStream = &std::cin;
    } else {
        ifstream * fileInput  = new ifstream(f);
        if (!fileInput->good()) {
            std::cerr << "Unable to open input file " << f << ": " << strerror (errno) << std::endl;
            exit(1);
        }

        inputStream = fileInput;
    }

    string line;
    getline(*inputStream, line);
    readClassical(*inputStream);
    readHierarchical(*inputStream);
    printSummary();
    if (isHtnModel) {
        generateMethodRepresentation();
    }
#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
    generateVectorRepresentation();
#endif
    if (f != "stdin"){
        ((ifstream*) inputStream)->close();
    }

// for debug:
#if DLEVEL == 5
    printActions();
    printMethods();
#endif
}

tuple<int*,int*,int**> Model::readConditionalIntList(string s, int& sizeA, int& sizeB, int*& sizeC){
    stringstream sStream(s);
    vector<int> v;
    vector<pair<vector<int>,int>> conds;
    int x;
    sStream >> x;
    while (x >= 0) {
        vector<int> c;
        for (int i = 0; i < x; i++){
            int y; sStream >> y;
            c.push_back(y);
        }

        // read the actual unguarded element
        sStream >> x;

        if (!c.size())
            v.push_back(x);
        else
            conds.push_back(make_pair(c,x));

        // read the beginning of the next
        sStream >> x;
    }
    sizeA = v.size();
    sizeB = conds.size();

    int* A = nullptr;
    int* B = nullptr;
    int** C = nullptr;
    if (sizeA) {
        A = new int[sizeA];
        for (int i = 0; i < sizeA; i++) A[i] = v[i];
    }

    if (sizeB){
        B = new int[sizeB];
        C = new int*[sizeB];
        for (int i = 0; i < sizeB; i++){
            B[i] = conds[i].second;
            C[i] = new int[conds[i].first.size()];
            for (size_t j = 0; j < conds[i].first.size(); j++) C[i][j] = conds[i].first[j];
        }
    }

    return make_tuple(A,B,C);
}


int* Model::readIntList(string s, int& size) {
    stringstream sStream(s);
    vector<int> v;
    int x;
    sStream >> x;
    while (x != -1) {
        if (x < 0) x++; // convert back to -i-1 instead of -i-2
        v.push_back(x);
        sStream >> x;
    }
    size = v.size();
    if (size == 0) {
        return nullptr;
    } else {
        int* res = new int[size];
        for (int i = 0; i < size; i++) {
            res[i] = v[i];
        }
        return res;
    }
}

void Model::printSummary() {
    cout << "- State has " << numStateBits << " bits divided into " << numVars
            << " mutex groups." << endl;
    cout << "- Domain contains " << numActions << " actions." << endl;
    if (isHtnModel) {
        cout << "- Domain contains " << numTasks << " tasks." << endl;
        cout << "- Domain contains " << numMethods << " methods." << endl;
    }
    cout << "- The initial state contains " << s0Size << " set bits." << endl;
    if (isHtnModel) {
        cout << "- The initial task is \"" << taskNames[initialTask] << "\"."
                << endl;
    }
    cout << "- State-based goal contains " << gSize << " bits." << endl;
}

void Model::printActions() {
    for (int i = 0; i < numActions; i++)
        printAction(i);
}
void Model::printAction(int aI) {
    cout << "action: " << taskNames[aI] << " :" << endl;
    cout << "   { ";
    for (int i = 0; i < numPrecs[aI]; i++) {
        if (i > 0)
            cout << " ";
        cout << factStrs[precLists[aI][i]];
    }
    cout << " }" << endl << "   { ";
    for (int i = 0; i < numAdds[aI]; i++) {
        if (i > 0)
            cout << " ";
        cout << factStrs[addLists[aI][i]];
    }
    cout << " }" << endl << "   { ";
    for (int i = 0; i < numDels[aI]; i++) {
        if (i > 0)
            cout << " ";
        cout << factStrs[delLists[aI][i]];
    }
    cout << " }" << endl << endl;
}
void Model::printActionsToFile(string file) {
    ofstream outf;
    outf.open(file);
    for (int i = 0; i < numActions; i++){
    outf << "action: " << taskNames[i] << " :" << endl;
    outf << "   { ";
    for (int j = 0; j < numPrecs[i]; j++) {
        if (j > 0)
            outf << " ";
        outf << factStrs[precLists[i][j]];
    }
    outf << " }" << endl << "   { ";
    for (int j = 0; j < numAdds[i]; j++) {
        if (j > 0)
            outf << " ";
        outf << factStrs[addLists[i][j]];
    }
    outf << " }" << endl << "   { ";
    for (int j = 0; j < numDels[i]; j++) {
        if (j > 0)
            outf << " ";
        outf << factStrs[delLists[i][j]];
    }
    outf << " }" << endl << endl;
    }
    outf.close();
}
void Model::printStateBitsToFile(string file) {
    ofstream outf;
    outf.open(file);
    outf << "VarNames: " << numVars << endl;
    for (int i = 0; i < numVars; i++){
      outf << i << " " << varNames[i] << " " << firstIndex[i] << " " << lastIndex[i] << endl;
    }
    outf << endl << "StateBits: " << numStateBits << endl;
    for (int i = 0; i < numStateBits; i++){
      outf << i << " " << factStrs[i] << endl;
    }
    outf.close();
}

void Model::printMethods() {
    for (int i = 0; i < numMethods; i++)
        printMethod(i);
}

void Model::printMethod(int mI) {
    cout << methodNames[mI] << endl;
    cout << "   @ " << taskNames[decomposedTask[mI]] << endl;
    for (int i = 0; i < numSubTasks[mI]; i++) {
        cout << "   " << i << " " << taskNames[subTasks[mI][i]] << endl;
    }
    for (int i = 0; i < numOrderings[mI]; i += 2) {
        cout << "   " << ordering[mI][i] << " < " << ordering[mI][i + 1]
                << endl;
    }
    cout << "   First tasks: ";
    for (int i = 0; i < numFirstTasks[mI]; i++) {
        if (i > 0)
            cout << " ";
        cout << methodsFirstTasks[mI][i];
    }
    cout << endl;
    cout << "   Last tasks: ";
    for (int i = 0; i < numLastTasks[mI]; i++) {
        if (i > 0)
            cout << " ";
        cout << methodsLastTasks[mI][i];
    }
    cout << endl;
    cout << "   Number of abstract first tasks: "
            << numFirstAbstractSubTasks[mI] << endl;
    cout << "   Number of primitive first tasks: " << numFirstPrimSubTasks[mI]
            << endl;
    cout << endl;
}

// temporal SCC information
int maxdfs; // counter for dfs
bool* U; // set of unvisited nodes
vector<int>* S; // stack
bool* containedS;
int* dfsI;
int* lowlink;

void Model::calcSCCs() {
    cout << "Calculate SCCs..." << endl;

    maxdfs = 0;
    U = new bool[numTasks];
    S = new vector<int>;
    containedS = new bool[numTasks];
    dfsI = new int[numTasks];
    lowlink = new int[numTasks];
    numSCCs = 0;
    taskToSCC = new int[numTasks];
    for (int i = 0; i < numTasks; i++) {
        U[i] = true;
        containedS[i] = false;
        taskToSCC[i] = -1;
    }

    tarjan(initialTask); // this works only if there is a single initial task and all tasks are connected to to that task

    sccSize = new int[numSCCs];
    for (int i = 0; i < numSCCs; i++)
        sccSize[i] = 0;
    numCyclicSccs = 0;
    for (int i = 0; i < numTasks; i++) {
        int j = taskToSCC[i];
        sccSize[j]++;
        if (sccSize[j] == 2)
            numCyclicSccs++;
    }
    cout << "- Number of SCCs: " << numSCCs << endl;

    // generate inverse mapping
    sccToTasks = new int*[numSCCs];
    int * currentI = new int[numSCCs];
    for (int i = 0; i < numSCCs; i++)
        currentI[i] = 0;
    for (int i = 0; i < numSCCs; i++) {
        sccToTasks[i] = new int[sccSize[i]];
    }
    for (int i = 0; i < numTasks; i++) {
        int scc = taskToSCC[i];
        sccToTasks[scc][currentI[scc]] = i;
        currentI[scc]++;
    }
    delete[] currentI;
    // search for sccs with size 1 that contain self-loops
    set<int> selfLoopSccs;
    for(int i = 0; i < numSCCs;i++){
        if(sccSize[i] == 1) {
            int task = sccToTasks[i][0];
            for(int mi =0; mi < numMethodsForTask[task]; mi++){
                int method = taskToMethods[task][mi];
                for(int ist = 0; ist < numSubTasks[method];ist++){
                    int subtask = subTasks[method][ist];
                    if(task == subtask){ // this is a self loop
                        selfLoopSccs.insert(i);
                    }
                }
            }
        }
    }
    numCyclicSccs += selfLoopSccs.size();
    numSccOneWithSelfLoops = selfLoopSccs.size();

    sccsCyclic = new int[numCyclicSccs];
    int j = 0;
    for (int i = 0; i < numSCCs; i++) {
        if (sccSize[i] > sccMaxSize)
            sccMaxSize = sccSize[i];
        if (sccSize[i] > 1) {
            sccsCyclic[j] = i;
            j++;
        }
    }
    for (std::set<int>::iterator it = selfLoopSccs.begin(); it != selfLoopSccs.end(); it++) {
        sccsCyclic[j] = *it;
        j++;
    }

    delete[] U;
    delete S;
    delete[] containedS;
    delete[] dfsI;
    delete[] lowlink;
}

void Model::tarjan(int v) {
    dfsI[v] = maxdfs;
    lowlink[v] = maxdfs; // v.lowlink <= v.dfs
    maxdfs++;

    S->push_back(v);
    containedS[v] = true;
    U[v] = false; // delete v from U

    for (int iM = 0; iM < numMethodsForTask[v]; iM++) { // iterate methods
        int m = taskToMethods[v][iM];
        for (int iST = 0; iST < numSubTasks[m]; iST++) { // iterate subtasks -> these are the adjacent nodes
            int v2 = subTasks[m][iST];
            if (U[v2]) {
                tarjan(v2);
                if (lowlink[v] > lowlink[v2]) {
                    lowlink[v] = lowlink[v2];
                }
            } else if (containedS[v2]) {
                if (lowlink[v] > dfsI[v2])
                    lowlink[v] = dfsI[v2];
            }
        }
    }

    if (lowlink[v] == dfsI[v]) { // root of an SCC
        int v2;
        do {
            v2 = S->back();
            S->pop_back();
            containedS[v2] = false;
            taskToSCC[v2] = numSCCs;
        } while (v2 != v);
        numSCCs++;
    }
}

void Model::calcSCCGraph() {
    calculatedSccs = true;
    set<int>** sccg = new set<int>*[numSCCs];
    for (int i = 0; i < numSCCs; i++)
        sccg[i] = new set<int>;

    for (int iT = 0; iT < numTasks; iT++) {
        int sccFrom = taskToSCC[iT];
        for (int iM = 0; iM < numMethodsForTask[iT]; iM++) {
            int m = taskToMethods[iT][iM];
            for (int iST = 0; iST < numSubTasks[m]; iST++) {
                int sccTo = taskToSCC[subTasks[m][iST]];
                if (sccFrom != sccTo) {
                    sccg[sccFrom]->insert(sccTo);
                }
            }
        }
    }
    if (numCyclicSccs == 0) {
        cout << "- The problem is acyclic" << endl;
    } else {
        cout << "- The problem is cyclic" << endl;
        cout << "- Number of cyclic SCCs: " << numCyclicSccs << endl;
        cout << "- Number of cyclic SCCs of size 1: " << numSccOneWithSelfLoops << endl;
    }

    // top-down mapping
    this->sccGnumSucc = new int[numSCCs];
    for (int i = 0; i < numSCCs; i++) {
        sccGnumSucc[i] = 0;
        for (int j = 0; j < numSCCs; j++) {
            if (sccg[i]->find(j) != sccg[i]->end()) {
                sccGnumSucc[i]++;
            }
        }
    }

    sccG = new int*[numSCCs];
    for (int i = 0; i < numSCCs; i++) {
        sccG[i] = new int[sccGnumSucc[i]];
        int k = 0;
        for (int j = 0; j < numSCCs; j++) {
            if (sccg[i]->find(j) != sccg[i]->end()) {
                sccG[i][k] = j;
                k++;
            }
        }
        assert(k == sccGnumSucc[i]);
    }

    // bottom-up mapping
    this->sccGnumPred = new int[numSCCs];
    for (int i = 0; i < numSCCs; i++) {
        sccGnumPred[i] = 0;
        for (int j = 0; j < numSCCs; j++) {
            if (sccg[j]->find(i) != sccg[j]->end()) {
                sccGnumPred[i]++;
            }
        }
    }

    sccGinverse = new int*[numSCCs];
    for (int i = 0; i < numSCCs; i++) {
        sccGinverse[i] = new int[sccGnumPred[i]];
        int k = 0;
        for (int j = 0; j < numSCCs; j++) {
            if (sccg[j]->find(i) != sccg[j]->end()) {
                sccGinverse[i][k] = j;
                k++;
            }
        }
        assert(k == sccGnumPred[i]);
    }

    // reachability
#ifdef MAINTAINREACHABILITY
#ifdef ALLTASKS
    int lastMaintained = this->numTasks;
#endif
#ifdef ONLYACTIONS
    int lastMaintained = this->numActions;
#endif
    this->numReachable = new int[numTasks];
    for (int i = 0; i < numTasks; i++) {
        this->numReachable[i] = -1;
    }
    this->reachable = new int*[numTasks];

    int * ready = new int[numSCCs];
    vector<int> stack;

    for (int i = 0; i < numSCCs; i++) {
        ready[i] = sccGnumSucc[i];
        if (sccGnumSucc[i] == 0)
            stack.push_back(i);
    }
    set<int> tReachable;
    int processedSCCs = 0;

    while (!stack.empty()) {
        int scc = stack.back();
        stack.pop_back();
        processedSCCs++;
        // calculate reachability
        tReachable.clear();
        for (int i = 0; i < sccSize[scc]; i++) {
            if (sccToTasks[scc][i] < lastMaintained)
                tReachable.insert(sccToTasks[scc][i]);
        }
        for (int i = 0; i < sccGnumSucc[scc]; i++) { // loop over successor sccs
            int nextSCC = sccG[scc][i];
            int someTask = sccToTasks[nextSCC][0]; // reachability is equal for all tasks in an scc -> we need only to consider one
            assert(numReachable[someTask] >= 0); // should have been set before because it belongs to a successor scc
            for (int k = 0; k < numReachable[someTask]; k++) {
                if (reachable[someTask][k] < lastMaintained)
                    tReachable.insert(reachable[someTask][k]);
            }
        }
        // write back reachability
        for (int i = 0; i < sccSize[scc]; i++) {
            int task = sccToTasks[scc][i];
            numReachable[task] = tReachable.size();
            reachable[task] = new int[numReachable[task]];
            int pos = 0;
            for (set<int>::iterator j = tReachable.begin();
                    j != tReachable.end(); j++, pos++) {
                reachable[task][pos] = *j;
            }
            assert(pos == numReachable[task]);
        }

        // update stack
        for (int i = 0; i < sccGnumPred[scc]; i++) {
            ready[sccGinverse[scc][i]]--;
            if (ready[sccGinverse[scc][i]] == 0) {
                stack.push_back(sccGinverse[scc][i]);
            }
        }
    }

/*
#ifdef MAINTAINREACHABILITYNOVEL
    // calculate inverse mapping
    set<int>** temp = new set<int>*[numTasks];
    for(int i =0; i < numTasks; i++) {
        temp[i] = new set<int>();
    }
    for(int t = numActions; t < numTasks; t++) {
        for(int it2 =0; it2 < numReachable[t]; it2++) {
            int t2 = reachable[t][it2];
            temp[t2]->insert(t);
        }
    }
    taskCanBeReachedFromNum = new int[numTasks];
    taskCanBeReachedFrom = new int*[numTasks];
    for(int t = 0; t < numTasks; t++) {
        taskCanBeReachedFrom[t] = new int[temp[t]->size()];
        int i = 0;
        for(int t2 : *temp[t]) {
            taskCanBeReachedFrom[t][i++] = t2;
        }
        delete temp[t];
    }
    delete[] temp;
#endif*/

    for (int i = 0; i < numSCCs; i++)
        delete sccg[i];
    delete[] sccg;

    assert(processedSCCs == numSCCs);
//#ifdef MAINTAINREACHABILITY
#ifdef ONLYACTIONS
    intSet.init(this->numActions);
#endif
#ifdef ALLTASKS
    intSet.init(this->numTasks);
#endif

#ifndef NDEBUG
    for (int i = 0; i < numSCCs; i++) {
        if (ready[i])
        cout << "FAIL " << i << endl;
        assert(ready[i] == 0);
    }
    for (int i = 0; i < numTasks; i++)
    assert(numReachable[i] >= 0);
#endif
  delete[] ready;
#endif
}

/*
bool Model::taskReachable(searchNode* tn, int t) {
    for(int i = 0; i < tn->numContainedTasks; i++) {
        int task = tn->containedTasks[i];
        if(iu.containsInt(reachable[task], 0, numReachable[task] - 1, t)) {
            return true;
        }
    }
    return false;
}*/


searchNode* Model::prepareTNi(const Model* htn) {
    // prepare initial node
    searchNode* tnI = new searchNode;
#if STATEREP == SRCOPY
    for (int i = 0; i < htn->numStateBits; i++) {
        tnI->state.push_back(false);
    }
    for (int i = 0; i < htn->s0Size; i++) {
        tnI->state[htn->s0List[i]] = true;
    }
#elif STATEREP == SRLIST
    tnI->stateSize = htn->s0Size;
    tnI->state = new int[htn->s0Size];
    for (int i = 0; i < htn->s0Size; i++) {
        tnI->state[i] = htn->s0List[i];
    }
#endif
    tnI->numPrimitive = 0;
    tnI->unconstraintPrimitive = nullptr;
    tnI->numAbstract = 1;
    tnI->unconstraintAbstract = new planStep*[1];
    tnI->unconstraintAbstract[0] = new planStep();
    tnI->unconstraintAbstract[0]->task = htn->initialTask;
    tnI->unconstraintAbstract[0]->pointersToMe = 1;
    tnI->unconstraintAbstract[0]->id = 0;
    tnI->unconstraintAbstract[0]->numSuccessors = 0;
    tnI->solution = nullptr;
    tnI->modificationDepth = 0;
    tnI->mixedModificationDepth = 0;

#ifdef TRACKTASKSINTN
    tnI->numContainedTasks = 1;
    tnI->containedTasks = new int[1];
    tnI->containedTaskCount = new int[1];
    tnI->containedTasks[0] = htn->initialTask;
    tnI->containedTaskCount[0] = 1;
#endif

    return tnI;
}


struct tOrMnode {
    bool isMethod = false;
    int id;
};

void Model::calcMinimalImpliedX() {

    timeval tp;
    gettimeofday(&tp, NULL);
    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    cout << "- Calculating minimal implied costs and distances";

    this->minImpliedCosts = new int[this->numTasks];
    this->minImpliedDistance = new int[this->numTasks];
    int* minImpliedCostsM = new int[this->numMethods];
    int* minImpliedDistanceM = new int[this->numMethods];

    for(int i = 0; i < this->numMethods; i++) {
        minImpliedCostsM[i] = 0;
        minImpliedDistanceM[i] = 0;
    }

    for(int i = 0; i < this->numTasks; i++){
        if(i < this->numActions){
            minImpliedCosts[i] = this->actionCosts[i];
            minImpliedDistance[i] = 1;
        } else {
            minImpliedCosts[i] = 0;
            minImpliedDistance[i] = 0;
        }
    }

    list<tOrMnode*> h;
    for(int i = 0; i < numActions; i++) {
        for(int j = 0; j < this->stToMethodNum[i]; j++) {
            int m = stToMethod[i][j];
            tOrMnode* nn = new tOrMnode();
            nn->id = m;
            nn->isMethod = true;
            h.push_back(nn);
        }
    }

    while(!h.empty()) {
        tOrMnode* n = h.front();
        h.pop_front();
        if (n->isMethod) {
            int cCosts = minImpliedCostsM[n->id];
            int cDist = minImpliedDistanceM[n->id];

            minImpliedCostsM[n->id] = 0;
            minImpliedDistanceM[n->id] = numSubTasks[n->id]; // number of tasks added to the stack
            int minAdditionalDistance = 0;
            for (int i = 0; i < this->numSubTasks[n->id]; i++) {
                int st = this->subTasksInOrder[n->id][i];
                if (st == decomposedTask[n->id]){
                  minAdditionalDistance = numSubTasks[n->id] - 1 - i;
                }
                minImpliedCostsM[n->id] += minImpliedCosts[st];
                int add = minImpliedDistance[st] - numSubTasks[n->id] + i;
                if (add > minAdditionalDistance){
                  minAdditionalDistance = add;
                }
            }
            minImpliedDistanceM[n->id] += minAdditionalDistance;
            bool changed = ((minImpliedCostsM[n->id] != cCosts)
                    || (minImpliedDistanceM[n->id] != cDist));
            if (changed) {
                tOrMnode* nn = new tOrMnode();
                nn->id = this->decomposedTask[n->id];
                nn->isMethod = false;
                h.push_back(nn);
            }
        } else { // is task
            int cCosts = minImpliedCosts[n->id];
            int cDist = minImpliedDistance[n->id];

            minImpliedCosts[n->id] = INT_MAX;
            minImpliedDistance[n->id] = INT_MAX;
            for (int i = 0; i < this->numMethodsForTask[n->id]; i++) {
                int m = this->taskToMethods[n->id][i];
                minImpliedCosts[n->id] = min(minImpliedCostsM[m], minImpliedCosts[n->id]);
                minImpliedDistance[n->id] = min(minImpliedDistanceM[m], minImpliedDistance[n->id]);
            }

            bool changed = ((minImpliedCosts[n->id] != cCosts)
                    || (minImpliedDistance[n->id] != cDist));
            if (changed) {
                for(int i = 0; i < this->stToMethodNum[n->id]; i++){
                    int m = stToMethod[n->id][i];
                    tOrMnode* nn = new tOrMnode();
                    nn->id = m;
                    nn->isMethod = true;
                    h.push_back(nn);
                }
            }
        }
        delete n;
    }
    delete[] minImpliedCostsM;
    delete[] minImpliedDistanceM;

    gettimeofday(&tp, NULL);
    long currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    cout << " (" << (currentT - startT) << " ms)" << endl;
    /*
    for (int i = 0 ; i < this->numTasks; i++) {
        cout << this->taskNames[i] << " c:" << minImpliedCosts[i] << " d:" << minImpliedDistance[i] << endl;
    }
    */
}

void Model::calcMinimalProgressionBound(bool to) {

    timeval tp;
    gettimeofday(&tp, NULL);
    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    cout << "- Calculating minimal Progression Bound";

    this->minImpliedPGB = new int[this->numTasks];
    int* minImpliedPGBM = new int[this->numMethods];

    for(int i = 0; i < this->numMethods; i++) {
        minImpliedPGBM[i] = 0;
    }

    for(int i = 0; i < this->numTasks; i++){
        if(i < this->numActions){
            minImpliedPGB[i] = 1;
        } else {
            minImpliedPGB[i] = 0;
        }
    }

    list<tOrMnode*> h;
    for(int i = 0; i < numActions; i++) {
        for(int j = 0; j < this->stToMethodNum[i]; j++) {
            int m = stToMethod[i][j];
            tOrMnode* nn = new tOrMnode();
            nn->id = m;
            nn->isMethod = true;
            h.push_back(nn);
        }
    }

    while(!h.empty()) {
        tOrMnode* n = h.front();
        h.pop_front();
        if (n->isMethod) {
            int cDist = minImpliedPGBM[n->id];
            int off = 0;
            if (to){
              off = firstNumTOPrimTasks[n->id];
            }

            minImpliedPGBM[n->id] = numSubTasks[n->id] - off; // number of tasks added to the stack
            if (minImpliedPGBM[n->id] < 1){
              minImpliedPGBM[n->id] = 1;
            }
            int minAdditionalDistance = 0;
            for (int i = 0; i < this->numSubTasks[n->id] - off; i++) {
              int st = this->subTasksInOrder[n->id][i];
              if (st == decomposedTask[n->id]){
                int k = numSubTasks[n->id] - 1 - i - off;
                if (minAdditionalDistance < k){
                  minAdditionalDistance = numSubTasks[n->id] - 1 - i - off;
                }
              }
              int add = minImpliedPGB[st] - numSubTasks[n->id] + i + off;
              if (add > minAdditionalDistance){
                minAdditionalDistance = add;
              }
            }
            minImpliedPGBM[n->id] += minAdditionalDistance;
            bool changed = ((minImpliedPGBM[n->id] != cDist));
            if (changed) {
                tOrMnode* nn = new tOrMnode();
                nn->id = this->decomposedTask[n->id];
                nn->isMethod = false;
                h.push_back(nn);
            }
        } else { // is task
            int cDist = minImpliedPGB[n->id];

            minImpliedPGB[n->id] = INT_MAX;
            for (int i = 0; i < this->numMethodsForTask[n->id]; i++) {
                int m = this->taskToMethods[n->id][i];
                minImpliedPGB[n->id] = min(minImpliedPGBM[m], minImpliedPGB[n->id]);
            }

            bool changed = ((minImpliedPGB[n->id] != cDist));
            if (changed) {
                for(int i = 0; i < this->stToMethodNum[n->id]; i++){
                    int m = stToMethod[n->id][i];
                    tOrMnode* nn = new tOrMnode();
                    nn->id = m;
                    nn->isMethod = true;
                    h.push_back(nn);
                }
            }
        }
        delete n;
    }
    delete[] minImpliedPGBM;

    gettimeofday(&tp, NULL);
    long currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    cout << " (" << (currentT - startT) << " ms)" << endl;
    /*
    for (int i = 0 ; i < this->numTasks; i++) {
        cout << this->taskNames[i] << " c:" << minImpliedCosts[i] << " d:" << minImpliedDistance[i] << endl;
    }
    */
}

  void Model::writeToPDDL(string dName, string pName) {
      ofstream dfile;
      dfile.open (dName);
      dfile << "(define (domain rc)" << endl;
      dfile << "  (:predicates ";
      for(int i = 0; i < this->numStateBits; i++) {
          dfile << "(" << su.cleanStr(this->factStrs[i]) << ")" << endl;
          if(i < this->numStateBits - 1)
              dfile << "               ";
      }
      dfile << "  )" << endl << endl;

      for(int i = numActions; i < numTasks; i++) {
          dfile << "  (:task " << su.cleanStr(this->taskNames[i]);
          dfile << " :parameters ())" << endl;
      }
      dfile << endl;

      for(int i = 0; i < numMethods; i++) {
          dfile << "  (:method " << su.cleanStr(this->methodNames[i]) << "_" << i << endl;
          dfile << "     :parameters ()" << endl;
          dfile << "     :task (" << su.cleanStr(this->taskNames[this->decomposedTask[i]]) << ")" << endl;
          dfile << "     :subtasks (and" << endl;
          for(int j = 0; j < numSubTasks[i]; j++) {
              dfile << "        (task" << j << " (" << su.cleanStr(taskNames[subTasks[i][j]]) << "))" << endl;
          }
          dfile << "     )" << endl;

      if (this->numOrderings[i]){
              dfile << "     :ordering (and" << endl;

              int j = 0;
              while(j < this->numOrderings[i]) {
                  dfile << "        (task" << this->ordering[i][j] << " < task" << this->ordering[i][j + 1] << ")" << endl;
                  j+= 2;
              }
              dfile << "     )" << endl;
      }

          dfile << "  )" << endl;
      }
      dfile << endl;

      for(int i = 0; i < this->numActions; i++) {
          dfile << "  (:action " << su.cleanStr(this->taskNames[i]) << endl;
          dfile << "     :parameters ()" << endl;
          if (this->numPrecs[i]){
          dfile << "     :precondition (and " << endl;
              for(int j = 0; j < this->numPrecs[i]; j++) {
                  dfile << "         (" << su.cleanStr(this->factStrs[this->precLists[i][j]]) << ")" << endl;
              }
              dfile << "     )" << endl;
      }
          if (this->numAdds[i] + this->numDels[i]){
          dfile << "     :effect (and " << endl;
              for(int j = 0; j < this->numAdds[i]; j++) {
                  dfile << "         (" << su.cleanStr(this->factStrs[this->addLists[i][j]]) << ")" << endl;
              }
              for(int j = 0; j < this->numDels[i]; j++) {
                  dfile << "         (not(" << su.cleanStr(this->factStrs[this->delLists[i][j]]) << "))" << endl;
              }
              dfile << "     )" << endl;
      }
          dfile << "  )" << endl;
          if (i < this->numActions - 1)
              dfile << endl;
      }
      dfile << ")" << endl;
      dfile.close();

      ofstream pfile;
      pfile.open(pName);
      pfile << "(define (problem p)" << endl;
      pfile << "   (:domain rc)" << endl;

      if(this->isHtnModel) {
          pfile << "   (:htn :parameters ()" << endl;
          pfile << "      :subtasks (and" << endl;
          pfile << "         (" << su.cleanStr(taskNames[this->initialTask]) << ")" << endl;
          pfile << "      )" << endl;
          pfile << "   )" << endl;
      }
      pfile << "   (:init" << endl;
      for(int i = 0; i < this->s0Size; i++) {
          pfile << "      (" << su.cleanStr(this->factStrs[this->s0List[i]]) << ")" << endl;
      }
      pfile << "   )" << endl;
      if (this->gSize){
      pfile << "   (:goal (and" << endl;
          for(int i = 0; i < this->gSize; i++) {
              pfile << "      (" << su.cleanStr(this->factStrs[this->gList[i]]) << ")" << endl;
          }
          pfile << "   ))" << endl;
  }
      pfile << ")" << endl;
      pfile.close();
  }

  void Model::checkFastDownwardPlan(string domain, string plan){

    std::istream * inputStream;

    ifstream * fileInput  = new ifstream(domain);
    if (!fileInput->good()) {
        std::cerr << "Unable to open input file " << domain << ": " << strerror (errno) << std::endl;
        exit(1);
    }

    inputStream = fileInput;

    string line;
    
    while (true){
      getline(*inputStream, line);
      int index = line.find("end_metric");
      if (index < line.length()){
        break;
      }
    }
    getline(*inputStream, line);
    getline(*inputStream, line);
    
    cerr << line << " variables" << endl;
    
    numVars = stoi(line);
    s0List = new int[numVars];
    while (true){
      getline(*inputStream, line);
      int index = line.find("begin_state");
      if (index < line.length()){
        break;
      }
    }
    cerr << "initializing variables" << endl;
    for (int i = 0; i < numVars; i++){
      getline(*inputStream, line);
      s0List[i] = stoi(line);
      
      cerr << "variable: " << i << " value: " << s0List[i] << endl;
      
    }
    cerr << endl;
                         
    while (true){
      getline(*inputStream, line);
      int index = line.find("begin_goal");
      if (index < line.length()){
        break;
      }
    }
    getline(*inputStream, line);
    gSize = stoi(line);
    gList = new int[numVars];
    for (int i = 0; i < numVars; i++){
      gList[i] = -1;
    }
    for (int i = 0; i < gSize; i++){
      getline(*inputStream, line);
      int index = line.find(' ');
      int a = stoi(line.substr(0, index));
      int b = stoi(line.substr(index + 1, line.length()));
      gList[a] = b;
      
      cerr << "goal: " << a << " value: " << b << endl;
      
    }
    getline(*inputStream, line);
    getline(*inputStream, line);
    getline(*inputStream, line);
    
    numActions = stoi(line);
    cerr << endl;
    cerr << "number of actions: " << numActions << endl;
    precLists = new int*[numActions];
    addLists = new int*[numActions];
    
    taskNames = new string[numActions];
    for (int i = 0; i < numActions; i++){
      precLists[i] = new int[numVars];
      addLists[i] = new int[numVars];
      for (int j = 0; j < numVars; j++){
        precLists[i][j] = -1;
        addLists[i][j] = -1;
      }
      while (true){
        getline(*inputStream, line);
        int index = line.find("begin_operator");
        if (index < line.length()){
          break;
        }
      }
      getline(*inputStream, line);
      taskNames[i] = line;
      if (line.substr(0, 14).compare("method(id[126]") == 0){
        cerr << "read method " << "method(id[126]" << endl;
        cerr << "read method " << line.substr(0, 14) << endl;
        cerr << "read method " << line << endl;
        return;
      }
      getline(*inputStream, line);
      int n = stoi(line);
      for (int j = 0; j < n; j++){
        getline(*inputStream, line);
        int index = line.find(' ');
        int a = stoi(line.substr(0, index));
        int b = stoi(line.substr(index + 1, line.length()));
        precLists[i][a] = b;
      }
      getline(*inputStream, line);
      n = stoi(line);
      for (int j = 0; j < n; j++){
        getline(*inputStream, line);
        int i1 = line.find(' ');
        int i2 = line.find(' ', i1 + 1);
        int i3 = line.find(' ', i2 + 1);
        int a = stoi(line.substr(i1 + 1, i2 - i1));
        int b = stoi(line.substr(i2 + 1, i3 - i2));
        int c = stoi(line.substr(i3 + 1, line.length()));
        precLists[i][a] = b;
        addLists[i][a] = c;
      }
    }
    fileInput->close();
    cerr << endl;
    
    fileInput  = new ifstream(plan);
    if (!fileInput->good()) {
        std::cerr << "Unable to open input file " << domain << ": " << strerror (errno) << std::endl;
        exit(1);
    }

    inputStream = fileInput;
    getline(*inputStream, line);

    int i = 0;
    while (line.find("; cost") > 0){
      cerr << "applying method " << i << ": " << line << endl;
      int index = -1;
      for (int j = 0; j < numActions; j++){
        if (taskNames[j].compare(line.substr(1, line.length() - 2)) == 0){
          index = j;
          break;
        }
      }
      if (index == -1){
        cerr << "no method with this name found" << endl;
        return;
      }
      cerr << "State" << endl;
      for (int j = 0; j < numVars; j++){
        bool correct = ((precLists[index][j] == -1) || (precLists[index][j] == s0List[j]));
        if (!correct){
          cerr << "cant apply method because current state is: " << s0List[j] << " but should be: " << precLists[index][j] << endl;
          return;
        }
        if (precLists[index][j] != -1){
          cerr << j << ": " << s0List[j] << " == " << precLists[index][j] << "   ";
        }
        if (addLists[index][j] != -1){
          cerr << j << ": " << s0List[j] << " -> " << addLists[index][j];
          s0List[j] = addLists[index][j];
        }
        if (precLists[index][j] != -1 || addLists[index][j] != -1){
          cerr << endl;
        }
      }
      getline(*inputStream, line);
      i++;
    }
    cerr << "finished applying methods!" << endl;
    cerr << "checking goal" << endl;
    for (int i = 0; i < numVars; i++){
      cerr << i << " " << s0List[i] << " " << gList[i] << endl;
    }
  }
  
  
  void Model::reorderTasks(bool warning){
    firstNumTOPrimTasks = new int[numMethods];
    subTasksInOrder = new int*[numMethods];
    hasNoLastTask = new bool[numMethods];
    for (int i = 0; i < numMethods; i++){
      firstNumTOPrimTasks[i] = 0;
      subTasksInOrder[i] = new int[numSubTasks[i]];
      hasNoLastTask[i] = false;
      if (numSubTasks[i] == 0){
        continue;
      }
      if (numSubTasks[i] == 1){
        if (isPrimitive[subTasks[i][0]]){
          firstNumTOPrimTasks[i] = 1;
        }
        subTasksInOrder[i][0] = subTasks[i][0];
        continue;
      }
      if (numOrderings[i] == 0){
		if (i != taskToMethods[initialTask][0] && warning){
			cout << "ATTENTION: Instance is not of the parallel sequences type. Search with the PSeq encoding might be incomplete." << endl;
		}
        for (int j = 0; j < numSubTasks[i]; j++){
          subTasksInOrder[i][j] = subTasks[i][numSubTasks[i] - 1 - j];
        }
        hasNoLastTask[i] = true;
        continue;
      }
      int* subs = new int[numSubTasks[i]];
      for (int j = 0; j < numSubTasks[i]; j++){
		bool valueSet = false;
        for (int k = 0; k < numSubTasks[i]; k++){
          bool notAlone = false;
          for (int l = 0; l < numOrderings[i] / 2; l++){
            bool alreadyUsed = false;
            bool ka = false;
            for (int m = 0; m < j; m++){
              if (k == subs[m]){
                ka = true;
                break;
              }
              if (ordering[i][2 * l] == subs[m]){
                alreadyUsed = true;
                break;
              }
            }
            if (ka){
              notAlone = true;
              break;
            }
            if (not alreadyUsed && ordering[i][2 * l + 1] == k){
              notAlone = true;
              break;
            }
          }
          if (not notAlone){
            if (not valueSet){
				subs[j] = k;
				valueSet = true;
			} else {
				cout << "ATTENTION: Instance is not of the parallel sequences type. Search with the PSeq encoding might be incomplete." << endl;
			}
          }
        }
      }

      // reordering tasks and adjusting restrictions
      for (int j = 0; j < numSubTasks[i]; j++){
        subTasksInOrder[i][j] = subTasks[i][numSubTasks[i] - 1 - subs[j]];
      }
      for (int j = 0; j < numOrderings[i]; j++){
        ordering[i][j] = numSubTasks[i] - 1 - subs[ordering[i][j]];
      }
      // evaluating number of primitive tasks in front
      for (int j = 0; j < numSubTasks[i]; j++){
        if (isPrimitive[subTasks[i][j]]){
          firstNumTOPrimTasks[i]++;
        }
        else {
          break;
        }
      }
      
      // checking if every method has a last task
      if (numOrderings[i] / 2 < numSubTasks[i] - 1){
        hasNoLastTask[i] = true;
        cerr << i << " has not enough constraints" << endl;
      }
      else {
        bool* lower = new bool[numSubTasks[i]];
        for (int j = 0; j < numSubTasks[i] - 1; j++){
          lower[j] = false;
        }
        lower[numSubTasks[i] - 1] = true;
        for (int j = 0; j < numSubTasks[i] - 1; j++){
          for (int k = 0; k < numOrderings[i] / 2; k++){
            if (lower[ordering[i][2 * k]]){
              lower[ordering[i][2 * k + 1]] = true;
            }
          }
        }
        for (int j = 0; j < numSubTasks[i]; j++){
          if (!lower[j]){
            hasNoLastTask[i] = true;
            cerr << i << " has no true last task" << endl;
            break;
          }
        }
        delete[] lower;
      }
      delete[] subs;
    }
  }

  void Model::sasPlus(){
    sasPlusBits = new bool[numVars];
    sasPlusOffset = new int[numVars];
    bitsToSP = new int[numStateBits];
    bitAlone = new bool[numStateBits];

    firstIndexSP = new int[numVars];
    lastIndexSP = new int[numVars];
    
    for (int i = 0; i < numStateBits; i++){
      bitsToSP[i] = 0;
      bitAlone[i] = false;
    }
    for (int i = 0; i < numVars; i++){
      sasPlusBits[i] = false;
      sasPlusOffset[i] = 0;
      firstIndexSP[i] = 0;
      lastIndexSP[i] = 0;
    }

    bool change = false;
    
    for (int i = 0; i < numVars; i++){
      if (i > 0){
        sasPlusOffset[i] = sasPlusOffset[i-1];
        if (sasPlusBits[i-1]){
          sasPlusOffset[i]++;
        }
      }
      firstIndexSP[i] = firstIndex[i] + sasPlusOffset[i];
      lastIndexSP[i] = lastIndex[i] + sasPlusOffset[i];
      if (firstIndex[i] == lastIndex[i]){
        change = true;
        sasPlusBits[i] = true;
        lastIndexSP[i]++;
      }
    }

    numStateBitsSP = lastIndexSP[numVars - 1] + 1;
    
    if (not(change)){
      cerr << "no change" << endl;
      return;
    }
    factStrsSP = new string[numStateBitsSP];
    for (int i = 0; i < numVars; i++){
      for (int j = 0; j < lastIndex[i] - firstIndex[i] + 1; j++){
		bitsToSP[firstIndex[i] + j] = firstIndex[i] + j + sasPlusOffset[i];
        factStrsSP[firstIndex[i] + j + sasPlusOffset[i]] = factStrs[firstIndex[i] + j];
      }
      if (sasPlusBits[i]){
        factStrsSP[firstIndex[i] + 1 + sasPlusOffset[i]] = string("no_") + string(factStrs[firstIndex[i]]);
        bitAlone[firstIndex[i]] = true;
      }
    }
    
    strictMutexesSP = new int*[numStrictMutexes];
    mutexesSP = new int*[numMutexes];
    
    for (int i = 0; i < numStrictMutexes; i++){
      strictMutexesSP[i] = new int[strictMutexesSize[i]];
      for (int j = 0; j < strictMutexesSize[i]; j++){
        strictMutexesSP[i][j] = bitsToSP[strictMutexes[i][j]];
      }
    }
    for (int i = 0; i < numMutexes; i++){
      mutexesSP[i] = new int[mutexesSize[i]];
      for (int j = 0; j < mutexesSize[i]; j++){
        mutexesSP[i][j] = bitsToSP[mutexes[i][j]];
      }
    }
    
    numPrecsSP = new int[numActions];
    numAddsSP = new int[numActions];
  
    precListsSP = new int*[numActions];
    addListsSP = new int*[numActions];
    
    for (int i = 0; i < numActions; i++){
      numPrecsSP[i] = numPrecs[i];
      precListsSP[i] = new int[numPrecsSP[i]];
      for (int j = 0; j < numPrecsSP[i]; j++){
        precListsSP[i][j] = bitsToSP[precLists[i][j]];
      }
      numAddsSP[i] = numAdds[i];
      for (int j = 0; j < numDels[i]; j++){
        if (bitAlone[delLists[i][j]]){
          numAddsSP[i]++;
        }
      }
      addListsSP[i] = new int[numAddsSP[i]];
      for (int j = 0; j < numAdds[i]; j++){
        addListsSP[i][j] = bitsToSP[addLists[i][j]];
      }
      int k = numAdds[i];
      for (int j = 0; j < numDels[i]; j++){
        if (bitAlone[delLists[i][j]]){
          addListsSP[i][k] = bitsToSP[delLists[i][j]] + 1;
          k++;
        }
      }
    }

    s0ListSP = new int[numVars];
    for (int i = 0; i < numVars; i++){
      s0ListSP[i] = lastIndexSP[i];
    }
    for (int i = 0; i < s0Size; i++){
      int j = 0;
      while (true){
        if (j >= s0Size || firstIndex[j] > s0List[i]){
          j--;
          break;
        }
        j++;
      }
      s0ListSP[j] = bitsToSP[s0List[i]];
    }
    gListSP = new int[gSize];
    for (int i = 0; i < gSize; i++){
       gListSP[i] = bitsToSP[gList[i]];
    }
    
    s0Size = numVars;
    numStateBits = numStateBitsSP;
    delete[] factStrs;
    factStrs = factStrsSP;
    
    delete[] firstIndex;
    firstIndex = firstIndexSP;
    delete[] lastIndex;
    lastIndex = lastIndexSP;
    delete[] strictMutexes;
    strictMutexes = strictMutexesSP;
    delete[] mutexes;
    mutexes = mutexesSP;
    delete[] precLists;
    precLists = precListsSP;
    delete[] addLists;
    addLists = addListsSP;
    delete[] numPrecs;
    numPrecs = numPrecsSP;
    delete[] numAdds;
    numAdds = numAddsSP;
    delete[] s0List;
    s0List = s0ListSP;
    delete[] gList;
    gList = gListSP;

  }

  int Model::htnToCond(int pgb) {
    // number of translated variables
    int n = pgb * (pgb - 1);
    numVarsTrans = numVars + pgb * 2 + n;

    // indizes for variables
    firstVarIndex = 0;
    firstTaskIndex = numVars;
    firstConstraintIndex = numVars + pgb;
    firstStackIndex = firstConstraintIndex + n;

    // indizes for names
    firstIndexTrans = new int[numVarsTrans];
    lastIndexTrans = new int[numVarsTrans];

    for (int i = 0; i < numVars; i++){
      firstIndexTrans[firstVarIndex+ i] = firstIndex[i];
      lastIndexTrans[firstVarIndex + i] = lastIndex[i];
    }
    
    for (int i = 0; i < pgb; i++){
      firstIndexTrans[firstTaskIndex + i] = lastIndexTrans[firstTaskIndex + i - 1] + 1;
      lastIndexTrans[firstTaskIndex + i] = firstIndexTrans[firstTaskIndex + i] + numTasks;
    }

    for (int i = 0; i < n; i++){
      firstIndexTrans[firstConstraintIndex + i] = lastIndexTrans[firstConstraintIndex + i - 1] + 1;
      lastIndexTrans[firstConstraintIndex + i] = firstIndexTrans[firstConstraintIndex + i] + 1;
    }

    for (int i = 0; i < pgb; i++){
      firstIndexTrans[firstStackIndex + i] = lastIndexTrans[firstStackIndex + i - 1] + 1;
      lastIndexTrans[firstStackIndex + i] = firstIndexTrans[firstStackIndex + i] + 1;
    }

    varNamesTrans = new string[numVarsTrans];
    for (int i = 0; i < numVarsTrans; i++){
        varNamesTrans[i] = "var_" + to_string(i);
    }

    numStateBitsTrans = lastIndexTrans[numVarsTrans - 1] + 1;

    factStrsTrans = new string[numStateBitsTrans];
    
    for (int i = 0; i < numStateBits; i++){
      factStrsTrans[firstIndexTrans[firstVarIndex] + i] = factStrs[i];
    }
    
    for (int i = firstTaskIndex; i < firstConstraintIndex; i++){
      factStrsTrans[firstIndexTrans[i]] = string("+task[point") + to_string(i-firstTaskIndex) + string(",noTask]")    ;
      for (int j = 0; j < lastIndexTrans[i] - firstIndexTrans[i]; j++){
          factStrsTrans[firstIndexTrans[i]+j + 1] = string("+task[point") + to_string(i - firstTaskIndex) + string(",task") + to_string(j) + ']';
      }
    }
    
    int j = 0;
    int k = 0;
    
    for (int i = firstConstraintIndex; i < numVarsTrans; i++){
      j++;
      if (k == j) {
        j++;
      }
      if (j == pgb) {
        j = 0;
        k++;
      }
      factStrsTrans[firstIndexTrans[i]] = string("+no_Constraint[")+ to_string(k) + ',' + to_string(j) + ']';
      factStrsTrans[firstIndexTrans[i] + 1] = string("+Constraint[") + to_string(k) + ',' + to_string(j) + ']';
    }
    
    for (int i = 0; i < pgb; i++){
      factStrsTrans[firstIndexTrans[i + firstStackIndex]] = string("+free[head")+ to_string(i) + ']';
      factStrsTrans[firstIndexTrans[i + firstStackIndex] + 1] = string("+occupied[head")+ to_string(i) + ']';
    }
    
    // Initial state
    s0SizeTrans = numVarsTrans-firstTaskIndex;
    s0ListTrans = new int[s0SizeTrans];
    s0ListTrans[0] = firstIndexTrans[firstTaskIndex] + initialTask + 1;
    for (int i = 1; i < s0SizeTrans; i++){
      s0ListTrans[i] = firstIndexTrans[firstTaskIndex + i];
    }
    
    s0ListTrans[firstStackIndex - firstTaskIndex] = firstIndexTrans[firstStackIndex] + 1;

    // goal state
    gSizeTrans = pgb;
    gListTrans = new int[pgb];
    for (int i = 0; i < pgb; i++){
      gListTrans[i] = firstIndexTrans[firstTaskIndex + i];
    }

    // transformed actions and methods
    methodIndexes = new int[numMethods + 1];
    methodIndexes[0] = 0;
    for (int i = 1; i < numMethods + 1; i++){
      int m = bin(pgb - 1, numSubTasks[i - 1] - 1);
      if (m < 1){
        m = 1;
      }
      if (m == INT_MAX){
        return -1;
      }
      methodIndexes[i] = methodIndexes[i - 1] + m;
    }
    
    numMethodsTrans = methodIndexes[numMethods];
    numMethodsTrans *= pgb;
    numActionsTrans = numActions * pgb + numMethodsTrans;
    firstMethodIndex = numActions * pgb;
    actionCostsTrans = new int[numActionsTrans];
    invalidTransActions = new bool[numActionsTrans];
    for (int i = 0; i < numActionsTrans; i++) {
      invalidTransActions[i] = false;
    }
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        actionCostsTrans[i * pgb + j] = actionCosts[i];
      }
    }
    for (int i = firstMethodIndex; i < numActionsTrans; i++) {
      actionCostsTrans[i] = 0;
    }
    
    // conditional adds
    numConditionalEffectsTrans = new int[numActionsTrans];
    effectConditionsTrans = new int**[numActionsTrans];
    numEffectConditionsTrans = new int*[numActionsTrans];
    effectsTrans = new int*[numActionsTrans];
    for (int i = 0; i < firstMethodIndex; i++) {
      numConditionalEffectsTrans[i] = 0;
      effectConditionsTrans[i] = nullptr;
      numEffectConditionsTrans[i] = nullptr;
      effectsTrans[i] = nullptr;
    }
    for (int i = 0; i < numMethods; i++) {
      for (int j = 0; j < pgb; j++){
        for (int k = 0; k < (methodIndexes[i + 1] - methodIndexes[i]); k++){
          int index = firstMethodIndex + methodIndexes[i] * pgb + j * (methodIndexes[i + 1] - methodIndexes[i]) + k;
          if (numSubTasks[i] > 1){
            numConditionalEffectsTrans[index] = (numSubTasks[i] - 1) * (pgb - 1) + numOrderings[i] / 2;
            effectConditionsTrans[index] = new int*[numConditionalEffectsTrans[index]];
            numEffectConditionsTrans[index] = new int[numConditionalEffectsTrans[index]];
            effectsTrans[index] = new int[numConditionalEffectsTrans[index]];
            for (int l = 0; l < numConditionalEffectsTrans[index]; l++){
              numEffectConditionsTrans[index][l] = 1;
              effectConditionsTrans[index][l] = new int[numEffectConditionsTrans[index][l]];
            }
          }
          else {
            numConditionalEffectsTrans[index] = 0;
            effectConditionsTrans[index] = nullptr;
            numEffectConditionsTrans[index] = nullptr;
            effectsTrans[index] = nullptr;
          }
        }
      }
    }

    // transformed actions
    numPrecsTrans = new int[numActionsTrans];
    numAddsTrans = new int[numActionsTrans];
    numDelsTrans = new int[numActionsTrans];
    precListsTrans = new int*[numActionsTrans];
    addListsTrans = new int*[numActionsTrans];
    delListsTrans = new int*[numActionsTrans];
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        int index = i * pgb + j;
        numPrecsTrans[index] = numPrecs[i] + pgb;
        numAddsTrans[index] = numAdds[i] + pgb + 1;
        precListsTrans[index] = new int[numPrecsTrans[index]];
        addListsTrans[index] = new int[numAddsTrans[index]];
        
        for (int k = 0; k < numPrecs[i]; k++){
          precListsTrans[index][k] = precLists[i][k];
        }
        for (int k = 0; k < numAdds[i]; k++){
          addListsTrans[index][k] = addLists[i][k];
        }
        for (int k = 0; k < pgb; k++){
          if (k < j){
            precListsTrans[index][numPrecs[i] + k] = firstIndexTrans[firstConstraintIndex + k * (pgb - 1) + j - 1];
          }
          else if (j == k){
            precListsTrans[index][numPrecs[i] + k] = firstIndexTrans[firstTaskIndex + j] + 1 + i;
          }
          else {
            precListsTrans[index][numPrecs[i] + k] = firstIndexTrans[firstConstraintIndex + k * (pgb - 1) + j];
          }
        }
        for (int k = 0; k < pgb - 1; k++){
          addListsTrans[index][numAdds[i] + k] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + k];
        }
        addListsTrans[index][numAdds[i] + pgb - 1] = firstIndexTrans[firstTaskIndex + j];
        addListsTrans[index][numAdds[i] + pgb] = firstIndexTrans[firstStackIndex + j];
      }
    }    

   
    taskToKill = new int[numMethods];
    for (int i = 0; i < numTasks; i++){
        for (int j = 0; j < numMethodsForTask[i]; j++){
            taskToKill[taskToMethods[i][j]] = i + 1;
        }
    }
    
    // transformed methods
    for (int i = 0; i < numMethods; i++) {
      int* subs = new int[numSubTasks[i] -1];
      for (int j = 0; j < pgb; j++){
        for (int k = 0; k < (methodIndexes[i + 1] - methodIndexes[i]); k++){
          int index = firstMethodIndex + methodIndexes[i] * pgb + j * (methodIndexes[i + 1] - methodIndexes[i]) + k;
          if (numSubTasks[i] == 0){
            numAddsTrans[index] = pgb + 1;
            numPrecsTrans[index] = pgb + 1;
          }
          else if (numSubTasks[i] == 1){
            numAddsTrans[index] = 1;
            numPrecsTrans[index] = pgb;
          }
          else {
            combination(subs, pgb - 1, numSubTasks[i] - 1, k);
            numAddsTrans[index] = numSubTasks[i] * 2 - 1;
            numPrecsTrans[index] = numSubTasks[i] + pgb + subs[numSubTasks[i] - 2];
          }
          
          precListsTrans[index] = new int[numPrecsTrans[index]];
          addListsTrans[index] = new int[numAddsTrans[index]];
          
          if (numSubTasks[i] > pgb){
            invalidTransActions[index] = true;
            continue;
          }
          
          for (int l = 0; l < pgb; l++){
            if (l < j){
              precListsTrans[index][l] = firstIndexTrans[firstConstraintIndex + l * (pgb - 1) + j - 1];
            }
            else if (j == l){
              precListsTrans[index][l] = firstIndexTrans[firstTaskIndex + j] + taskToKill[i];
            }
            else {
              precListsTrans[index][l] = firstIndexTrans[firstConstraintIndex + l * (pgb - 1) + j];
            }
          }
          
          if (numSubTasks[i] == 0){
            addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j];
            for (int l = 0; l < pgb - 1; l++){
              addListsTrans[index][l + 1] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + l];
            }
            addListsTrans[index][pgb] = firstIndexTrans[firstStackIndex + j];
          }
          else if (numSubTasks[i] == 1){
            addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
          }
          else {
            addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
            for (int m = 0; m < subs[numSubTasks[i] - 2] + 1; m++){
              int off = 0;
              if (m >= j){
                off = 1;
              }
              precListsTrans[index][numSubTasks[i] + pgb - 1 + m] = firstIndexTrans[firstStackIndex + m + off] + 1;
            }
            for (int l = 0; l < numSubTasks[i] - 1; l++){
              int off = 0;
              if (subs[l] >= j){
                off = 1;
                subs[l]++;
              }
              precListsTrans[index][l + pgb] = firstIndexTrans[firstTaskIndex + subs[l]];
              precListsTrans[index][numSubTasks[i] + pgb - 1 + subs[l] - off] = firstIndexTrans[firstStackIndex + subs[l]];
              addListsTrans[index][l + 1] = firstIndexTrans[firstTaskIndex + subs[l]] + 1 + subTasksInOrder[i][l + 1];
              addListsTrans[index][l + numSubTasks[i]] = firstIndexTrans[firstStackIndex + subs[l]] + 1;
              
              for (int m = 0; m < pgb - 1; m++){
                effectConditionsTrans[index][l * (pgb - 1) + m][0] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + m] + 1;
                effectsTrans[index][l * (pgb - 1) + m] = firstIndexTrans[firstConstraintIndex + subs[l] * (pgb - 1) + m] + 1;
              }
            }
            for (int l = 0; l < numOrderings[i] / 2; l++){
              int first = ordering[i][2 * l] - 1;
              int second = ordering[i][2 * l + 1] - 1;
              if (first < 0){
                first = j;
              }
              else {
                first = subs[first];
              }
              if (second < 0){
                second = j;
              }
              else {
                second = subs[second];
              }
              effectConditionsTrans[index][(pgb - 1) * (numSubTasks[i] - 1) + l][0] = firstIndexTrans[firstConstraintIndex + first * (pgb - 1) + second];
              effectsTrans[index][(pgb - 1) * (numSubTasks[i] - 1) + l] = firstIndexTrans[firstConstraintIndex + first * (pgb - 1) + second] + 1;
            }
          }
        }
      }
      delete[] subs;
    }
    numInvalidTransActions = 0;
    for (int i = 0; i < numActionsTrans; i++) {
      if (invalidTransActions[i]){
        numInvalidTransActions++;
      }
    }
    
    // names
    actionNamesTrans = new string[numActionsTrans];
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        actionNamesTrans[i * pgb + j] = "primitive(id[" + to_string(i) + "],head[" + to_string(j) + "]): " + taskNames[i];
      }
    }
    for (int i = 0; i < numMethods; i++) {
      int* subs = new int[numSubTasks[i] - 1];
      for (int j = 0; j < pgb; j++){
        for (int k = 0; k < (methodIndexes[i + 1] - methodIndexes[i]); k++){
          int index = firstMethodIndex + methodIndexes[i] * pgb + j * (methodIndexes[i + 1] - methodIndexes[i]) + k;
          if (invalidTransActions[index]){
            continue;
          }
          actionNamesTrans[index] = "method(id[" + to_string(i) + "],head[" + to_string(j);
          if (numSubTasks[i] > 0){
            combination(subs, pgb - 1, numSubTasks[i] - 1, k);
            actionNamesTrans[index] += "],subtasks[" + to_string(j);
            for (int l = 0; l < numSubTasks[i] - 1; l++){
              int off = subs[l];
              if (off >= j){
                off++;
              }
              if (l < numSubTasks[i] - 1){
                actionNamesTrans[index] += ",";
              }
              actionNamesTrans[index] += to_string(off);
            }
            actionNamesTrans[index] += "]): ";
            actionNamesTrans[index] += methodNames[i];
          }
        }
      }
    }
    return 0;
  }

  int Model::htnToCondSorted(int pgb) {
    // number of translated variables
    int n = pgb * (pgb - 1) / 2;
    numVarsTrans = numVars + pgb * 3 + n;

    // indizes for variables
    firstVarIndex = 0;
    firstTaskIndex = numVars;
    firstConstraintIndex = numVars + pgb;
    firstStackIndex = firstConstraintIndex + n;
    int firstMoveIndex = firstStackIndex + pgb;
    numInvalidTransActions = 0;

    // indizes for names
    firstIndexTrans = new int[numVarsTrans];
    lastIndexTrans = new int[numVarsTrans];

    for (int i = 0; i < numVars; i++){
      firstIndexTrans[firstVarIndex+ i] = firstIndex[i];
      lastIndexTrans[firstVarIndex + i] = lastIndex[i];
    }
    
    for (int i = 0; i < pgb; i++){
      firstIndexTrans[firstTaskIndex + i] = lastIndexTrans[firstTaskIndex + i - 1] + 1;
      lastIndexTrans[firstTaskIndex + i] = firstIndexTrans[firstTaskIndex + i] + numTasks;
    }

    for (int i = 0; i < n; i++){
      firstIndexTrans[firstConstraintIndex + i] = lastIndexTrans[firstConstraintIndex + i - 1] + 1;
      lastIndexTrans[firstConstraintIndex + i] = firstIndexTrans[firstConstraintIndex + i] + 1;
    }

    for (int i = 0; i < pgb; i++){
      firstIndexTrans[firstStackIndex + i] = lastIndexTrans[firstStackIndex + i - 1] + 1;
      lastIndexTrans[firstStackIndex + i] = firstIndexTrans[firstStackIndex + i] + 1;
    }
    for (int i = 0; i < pgb; i++){
      firstIndexTrans[firstMoveIndex + i] = lastIndexTrans[firstMoveIndex + i - 1] + 1;
      lastIndexTrans[firstMoveIndex + i] = firstIndexTrans[firstMoveIndex + i] + 1;
    }

    varNamesTrans = new string[numVarsTrans];
    for (int i = 0; i < numVarsTrans; i++){
        varNamesTrans[i] = "var_" + to_string(i);
    }

    numStateBitsTrans = lastIndexTrans[numVarsTrans - 1] + 1;

    factStrsTrans = new string[numStateBitsTrans];
    
    for (int i = 0; i < numStateBits; i++){
      factStrsTrans[firstIndexTrans[firstVarIndex] + i] = factStrs[i];
    }
    
    for (int i = firstTaskIndex; i < firstConstraintIndex; i++){
      factStrsTrans[firstIndexTrans[i]] = string("+task[point") + to_string(i-firstTaskIndex) + string(",noTask]")    ;
      for (int j = 0; j < lastIndexTrans[i] - firstIndexTrans[i]; j++){
          factStrsTrans[firstIndexTrans[i]+j + 1] = string("+task[point") + to_string(i - firstTaskIndex) + string(",task") + to_string(j) + ']';
      }
    }
    
    int j = 0;
    int k = 1;
    int index = firstConstraintIndex;
    
    for (int i = 0; i < n; i++){
      factStrsTrans[firstIndexTrans[index]] = string("+no_Constraint[")+ to_string(k) + ',' + to_string(j) + ']';
      factStrsTrans[firstIndexTrans[index] + 1] = string("+Constraint[") + to_string(k) + ',' + to_string(j) + ']';
      j++;
      index++;
      if (j >= k){
        k++;
        j = 0;
      }
    }
    
    for (int i = 0; i < pgb; i++){
      factStrsTrans[firstIndexTrans[i + firstStackIndex]] = string("+free[head")+ to_string(i) + ']';
      factStrsTrans[firstIndexTrans[i + firstStackIndex] + 1] = string("+occupied[head")+ to_string(i) + ']';
    }
    
    for (int i = 0; i < pgb; i++){
      factStrsTrans[firstIndexTrans[i + firstMoveIndex]] = string("+immovable[head")+ to_string(i) + ']';
      factStrsTrans[firstIndexTrans[i + firstMoveIndex] + 1] = string("+movable[head")+ to_string(i) + ']';
    }
    // Initial state
    s0SizeTrans = numVarsTrans-firstTaskIndex;
    s0ListTrans = new int[s0SizeTrans];
    s0ListTrans[0] = firstIndexTrans[firstTaskIndex] + initialTask + 1;
    for (int i = 1; i < s0SizeTrans; i++){
      s0ListTrans[i] = firstIndexTrans[firstTaskIndex + i];
    }
    
    s0ListTrans[firstStackIndex - firstTaskIndex] = firstIndexTrans[firstStackIndex] + 1;

    // goal state
    gSizeTrans = pgb;
    gListTrans = new int[pgb];
    for (int i = 0; i < pgb; i++){
      gListTrans[i] = firstIndexTrans[firstTaskIndex + i];
    }

    // transformed actions and methods
    /*
    methodIndexes = new int[numMethods + 1];
    methodIndexes[0] = 0;
    for (int i = 1; i < numMethods + 1; i++){
      int m = bin(pgb - 1, numSubTasks[i - 1] - 1);
      if (m < 1){
        m = 1;
      }
      if (m == INT_MAX){
        return -1;
      }
      methodIndexes[i] = methodIndexes[i - 1] + m;
    }
    */

    numActionsTrans = (numActions + numMethods) * pgb + numTasks * (pgb - 1);
    firstMethodIndex = numActions * pgb;
    int methodTaskIndex = (numActions + numMethods) * pgb;
    actionCostsTrans = new int[numActionsTrans];
    invalidTransActions = new bool[numActionsTrans];
    for (int i = 0; i < numActionsTrans; i++) {
      invalidTransActions[i] = false;
    }
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        actionCostsTrans[i * pgb + j] = actionCosts[i];
      }
    }
    for (int i = firstMethodIndex; i < numActionsTrans; i++) {
      actionCostsTrans[i] = 0;
    }
    
    // conditional adds
    numConditionalEffectsTrans = new int[numActionsTrans];
    effectConditionsTrans = new int**[numActionsTrans];
    numEffectConditionsTrans = new int*[numActionsTrans];
    effectsTrans = new int*[numActionsTrans];
    for (int i = 0; i < firstMethodIndex; i++) {
      numConditionalEffectsTrans[i] = 0;
      effectConditionsTrans[i] = nullptr;
      numEffectConditionsTrans[i] = nullptr;
      effectsTrans[i] = nullptr;
    }
    for (int i = 0; i < numMethods; i++) {
      for (int j = 0; j < pgb; j++){
        int index = firstMethodIndex + i * pgb + j;
        if (numSubTasks[i] > 1){
          numConditionalEffectsTrans[index] = (numSubTasks[i] - 1) * j + numOrderings[i] / 2;
          effectConditionsTrans[index] = new int*[numConditionalEffectsTrans[index]];
          numEffectConditionsTrans[index] = new int[numConditionalEffectsTrans[index]];
          effectsTrans[index] = new int[numConditionalEffectsTrans[index]];
          for (int l = 0; l < numConditionalEffectsTrans[index]; l++){
            numEffectConditionsTrans[index][l] = 1;
            effectConditionsTrans[index][l] = new int[numEffectConditionsTrans[index][l]];
          }
        }
        else {
          numConditionalEffectsTrans[index] = 0;
          effectConditionsTrans[index] = nullptr;
          numEffectConditionsTrans[index] = nullptr;
          effectsTrans[index] = nullptr;
        }
      }
    }
    index = methodTaskIndex;
    for (int i = 0; i < numTasks; i++) {
      for (int j = 0; j < (pgb - 1); j++){
        numConditionalEffectsTrans[index] = pgb;
        if (j > 0){
          numConditionalEffectsTrans[index] = pgb + 2;
        }
        effectConditionsTrans[index] = new int*[numConditionalEffectsTrans[index]];
        numEffectConditionsTrans[index] = new int[numConditionalEffectsTrans[index]];
        effectsTrans[index] = new int[numConditionalEffectsTrans[index]];
        for (int l = 0; l < numConditionalEffectsTrans[index]; l++){
          numEffectConditionsTrans[index][l] = 1;
          effectConditionsTrans[index][l] = new int[numEffectConditionsTrans[index][l]];
        }
        index++;
      }
    }

    // transformed actions
    numPrecsTrans = new int[numActionsTrans];
    numAddsTrans = new int[numActionsTrans];
    numDelsTrans = new int[numActionsTrans];
    precListsTrans = new int*[numActionsTrans];
    addListsTrans = new int*[numActionsTrans];
    delListsTrans = new int*[numActionsTrans];
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        int index = i * pgb + j;
        numPrecsTrans[index] = numPrecs[i] + pgb;
        numAddsTrans[index] = numAdds[i] + 2 + j;
        precListsTrans[index] = new int[numPrecsTrans[index]];
        addListsTrans[index] = new int[numAddsTrans[index]];
        for (int k = 0; k < numPrecs[i]; k++){
          precListsTrans[index][k] = precLists[i][k];
        }
        for (int k = 0; k < numAdds[i]; k++){
          addListsTrans[index][k] = addLists[i][k];
        }
        for (int k = 0; k < pgb; k++){
          if (k < j){
            precListsTrans[index][numPrecs[i] + k] = firstIndexTrans[firstStackIndex + k] + 1;
            addListsTrans[index][numAdds[i] + k] = firstIndexTrans[firstConstraintIndex + j * (j - 1) / 2 + k];
          }
          else if (k > j) {
            precListsTrans[index][numPrecs[i] + k] = firstIndexTrans[firstConstraintIndex + k * (k - 1) / 2 + j];
          }
          else {
            precListsTrans[index][numPrecs[i] + k] = firstIndexTrans[firstTaskIndex + j] + i + 1;
          }
        }
        addListsTrans[index][numAdds[i] + j] = firstIndexTrans[firstTaskIndex + j];
        addListsTrans[index][numAdds[i] + j + 1] = firstIndexTrans[firstStackIndex + j];
      }
    }    

   
    taskToKill = new int[numMethods];
    for (int i = 0; i < numTasks; i++){
      for (int j = 0; j < numMethodsForTask[i]; j++){
        taskToKill[taskToMethods[i][j]] = i + 1;
      }
    }
    
    // transformed methods
    for (int i = 0; i < numMethods; i++) {
      for (int j = 0; j < pgb; j++){
        int index = firstMethodIndex + i * pgb + j;
        if (numSubTasks[i] == 0){
          numAddsTrans[index] = 2 + j;
          numPrecsTrans[index] = pgb;
        }
        else if (numSubTasks[i] == 1){
          numAddsTrans[index] = 1;
          numPrecsTrans[index] = pgb;
        }
        else {
          numAddsTrans[index] = numSubTasks[i] * 2 - 1;
          numPrecsTrans[index] = numSubTasks[i] - 1 + pgb;
        }
        precListsTrans[index] = new int[numPrecsTrans[index]];
        addListsTrans[index] = new int[numAddsTrans[index]];

        if (numSubTasks[i] + j > pgb){
          invalidTransActions[index] = true;
          continue;
        }
        
        for (int k = 0; k < pgb; k++){
          if (k < j){
            precListsTrans[index][k] = firstIndexTrans[firstStackIndex + k] + 1;
          }
          else if (k > j) {
            precListsTrans[index][k] = firstIndexTrans[firstConstraintIndex + k * (k - 1) / 2 + j];
          }
          else {
            precListsTrans[index][k] = firstIndexTrans[firstTaskIndex + j] + taskToKill[i];
          }
        }
        
        if (numSubTasks[i] == 0){
          for (int k = 0; k < j; k++){
            addListsTrans[index][k] = firstIndexTrans[firstConstraintIndex + j * (j - 1) / 2 + k];
          }
          addListsTrans[index][j] = firstIndexTrans[firstTaskIndex + j];
          addListsTrans[index][j + 1] = firstIndexTrans[firstStackIndex + j];
        }
        else if (numSubTasks[i] == 1){
          addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
        }
        else {
          addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
          for (int l = 0; l < numSubTasks[i] - 1; l++){
            precListsTrans[index][l + pgb] = firstIndexTrans[firstTaskIndex + pgb - 1 - l];
            addListsTrans[index][l + 1] = firstIndexTrans[firstTaskIndex + pgb - numSubTasks[i] + 1 + l] + 1 + subTasksInOrder[i][l + 1];
            addListsTrans[index][l + numSubTasks[i]] = firstIndexTrans[firstStackIndex + pgb - 1 - l] + 1;
            for (int m = 0; m < j; m++){
              effectConditionsTrans[index][l * j + m][0] = firstIndexTrans[firstConstraintIndex + j * (j - 1) / 2 + m] + 1;
              effectsTrans[index][l * j + m] = firstIndexTrans[firstConstraintIndex + (pgb - l - 1) * (pgb - l - 2) / 2 + m] + 1;
            }
          }
          for (int l = 0; l < numOrderings[i] / 2; l++){
            int first = ordering[i][2 * l] - 1;
            int second = ordering[i][2 * l + 1] - 1;
            if (first < 0){
              first = j;
            }
            else {
              first = pgb - numSubTasks[i] + first + 1;
            }
            if (second < 0){
              second = j;
            }
            else {
              second = pgb - numSubTasks[i] + second + 1;
            }
            effectConditionsTrans[index][j * (numSubTasks[i] - 1) + l][0] = firstIndexTrans[firstConstraintIndex + first * (first - 1) / 2 + second];
            effectsTrans[index][j * (numSubTasks[i] - 1) + l] = firstIndexTrans[firstConstraintIndex + first * (first - 1) / 2 + second] + 1;
          }
        }
      }
    }

    // sort queue
    index = methodTaskIndex;
    for (int i = 0; i < numTasks; i++) {
      for (int j = 1; j < pgb; j++){
        numAddsTrans[index] = 5 + pgb - 2;
        numPrecsTrans[index] = 4 + pgb - 2;
        
        precListsTrans[index] = new int[numPrecsTrans[index]];
        addListsTrans[index] = new int[numAddsTrans[index]];

        precListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + i;
        precListsTrans[index][1] = firstIndexTrans[firstStackIndex + j] + 1;
        precListsTrans[index][2] = firstIndexTrans[firstTaskIndex + j - 1];
        precListsTrans[index][3] = firstIndexTrans[firstStackIndex + j - 1];

        addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j];
        addListsTrans[index][1] = firstIndexTrans[firstStackIndex + j];
        addListsTrans[index][2] = firstIndexTrans[firstTaskIndex + j - 1] + 1 + i;
        addListsTrans[index][3] = firstIndexTrans[firstStackIndex + j - 1] + 1;
        addListsTrans[index][4] = firstIndexTrans[firstMoveIndex + j];
        
        for (int l = 0; l < pgb - 2; l++){
          if (l < j){
            precListsTrans[index][4 + l] = firstIndexTrans[firstMoveIndex + l];
          }
          else {
            precListsTrans[index][4 + l] = firstIndexTrans[firstMoveIndex + l + 1];
          }
          if (l < j - 1){
            addListsTrans[index][5 + l] = firstIndexTrans[firstConstraintIndex + j * (j - 1) / 2 + l];
            effectConditionsTrans[index][l][0] = firstIndexTrans[firstConstraintIndex + j * (j - 1) / 2 + l] + 1;
            effectsTrans[index][l] = firstIndexTrans[firstConstraintIndex + (j - 1) * (j - 2) / 2 + l] + 1;
          }
          else {
            addListsTrans[index][5 + l] = firstIndexTrans[firstConstraintIndex + (l + 2) * (l + 1) / 2 + j];
            effectConditionsTrans[index][l][0] = firstIndexTrans[firstConstraintIndex + (l + 2) * (l + 1) / 2 + j] + 1;
            effectsTrans[index][l] = firstIndexTrans[firstConstraintIndex + (l + 2) * (l + 1) / 2 + j - 1] + 1;
          }
        }
        // ensure new immovability
        if (j > 1){
          effectConditionsTrans[index][pgb - 2][0] = firstIndexTrans[firstStackIndex + j - 2];
          effectsTrans[index][pgb - 2] = firstIndexTrans[firstMoveIndex + j - 1] + 1;
          effectConditionsTrans[index][pgb - 1][0] = firstIndexTrans[firstStackIndex + j - 2] + 1;
          effectsTrans[index][pgb - 1] = firstIndexTrans[firstMoveIndex + j - 1];
        }
        
        index++;
      }
    }

    for (int i = 0; i < numActionsTrans; i++) {
      if (invalidTransActions[i]){
        numInvalidTransActions++;
      }
    }
    
    // names
    actionNamesTrans = new string[numActionsTrans];
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        actionNamesTrans[i * pgb + j] = "primitive(id[" + to_string(i) + "],head[" + to_string(j) + "]): " + taskNames[i];
      }
    }
    for (int i = 0; i < numMethods; i++) {
      for (int j = 0; j < pgb; j++){
        int index = firstMethodIndex + i * pgb + j;
        if (invalidTransActions[index]){
          continue;
        }
        actionNamesTrans[index] = "method(id[" + to_string(i) + "],head[" + to_string(j);
        if (numSubTasks[i] > 0){
          actionNamesTrans[index] += "],subtasks[" + to_string(j);
          for (int l = 0; l < numSubTasks[i] - 1; l++){
            if (l < numSubTasks[i] - 1){
              actionNamesTrans[index] += ",";
            }
            actionNamesTrans[index] += to_string(pgb - numSubTasks[i] + 1 + l);
          }
          actionNamesTrans[index] += "]): ";
          actionNamesTrans[index] += methodNames[i];
        }
      }
    }
    
    index = methodTaskIndex;
    for (int i = 0; i < numTasks; i++) {
      for (int j = 1; j < pgb; j++){
        actionNamesTrans[index] = "move(task[" + to_string(i) + "],from[" + to_string(j) + "],to[" + to_string(j - 1) + "]";
        index++;
      }
    }
    return numActionsTrans;
  }
   
 int Model::htnToStrips(int pgb) {
    // number of translated variables
    int n = pgb * (pgb - 1);
    numVarsTrans = numVars + 2 * pgb + n;

    // indizes for variables
    firstVarIndex = 0;
    firstTaskIndex = numVars;
    firstConstraintIndex = numVars + pgb;
    firstStackIndex = firstConstraintIndex + n;

    // indizes for names
    firstIndexTrans = new int[numVarsTrans];
    lastIndexTrans = new int[numVarsTrans];

    for (int i = 0; i < numVars; i++){
      firstIndexTrans[firstVarIndex+ i] = firstIndex[i];
      lastIndexTrans[firstVarIndex+i] = lastIndex[i];
    }
    
    for (int i = 0; i < pgb; i++){
      firstIndexTrans[firstTaskIndex + i] = lastIndexTrans[firstTaskIndex + i - 1] + 1;
      lastIndexTrans[firstTaskIndex + i] = firstIndexTrans[firstTaskIndex + i] + numTasks+1;
    }

    for (int i = 0; i < n; i++){
      firstIndexTrans[firstConstraintIndex + i] = lastIndexTrans[firstConstraintIndex + i - 1] + 1;
      lastIndexTrans[firstConstraintIndex + i] = firstIndexTrans[firstConstraintIndex + i] + 1;
    }

    for (int i = 0; i < pgb; i++){
      firstIndexTrans[firstStackIndex + i] = lastIndexTrans[firstStackIndex + i - 1] + 1;
      lastIndexTrans[firstStackIndex + i] = firstIndexTrans[firstStackIndex + i] + 1;
    }

    varNamesTrans = new string[numVarsTrans];
    

    numStateBitsTrans = lastIndexTrans[numVarsTrans - 1] + 1;

    factStrsTrans = new string[numStateBitsTrans];
    for (int i = 0; i < numStateBits; i++){
      factStrsTrans[firstIndexTrans[firstVarIndex] + i] = factStrs[i];
    }
    for (int i = firstTaskIndex; i < firstConstraintIndex; i++){
      factStrsTrans[firstIndexTrans[i]] = string("+task[head") + to_string(i-firstTaskIndex) + string(",noTask]")    ;
      factStrsTrans[firstIndexTrans[i] + 1] = string("+task[head") + to_string(i-firstTaskIndex) + string(",emptyTask]")    ;
      for (int j = 0; j < lastIndexTrans[i] - firstIndexTrans[i] -1 ; j++){
        factStrsTrans[firstIndexTrans[i]+j + 1] = string("+task[head") + to_string(i - firstTaskIndex) + string(",task") + to_string(j) + ']';
      }
    }
    int j = 0;
    int k = 0;
    for (int i = firstConstraintIndex; i < firstStackIndex; i++){
      j++;
      if (k == j) {
        j++;
      }
      if (j == pgb) {
        j = 0;
        k++;
      }
      factStrsTrans[firstIndexTrans[i]] = string("+no_Constraint[")+ to_string(k) + ',' + to_string(j) + ']';
      factStrsTrans[firstIndexTrans[i] + 1] = string("+Constraint[") + to_string(k) + ',' + to_string(j) + ']';
    }
    for (int i = 0; i < pgb; i++){
      factStrsTrans[firstIndexTrans[i + firstStackIndex]] = string("+free[head")+ to_string(i) + ']';
      factStrsTrans[firstIndexTrans[i + firstStackIndex] + 1] = string("+occupied[head")+ to_string(i) + ']';
    }
    for (int i = 0; i < numVarsTrans; i++){
        varNamesTrans[i] = "var_" + to_string(i);
    }

    
    // Initial state
    s0SizeTrans = numVarsTrans-firstTaskIndex;
    s0ListTrans = new int[s0SizeTrans];
    s0ListTrans[0] = firstIndexTrans[firstTaskIndex] + initialTask + 2;
    for (int i = 1; i < s0SizeTrans; i++){
      s0ListTrans[i] = firstIndexTrans[firstTaskIndex + i];
    }
    s0ListTrans[firstStackIndex - firstTaskIndex] = firstIndexTrans[firstStackIndex] + 1;

    // goal state
    gSizeTrans = pgb;
    gListTrans = new int[pgb];
    for (int i = 0; i < pgb; i++){
      gListTrans[i] = firstIndexTrans[firstTaskIndex + i];
    }

    // empty tasks
    numEmptyTasks = pgb;
    emptyTaskNames = new string[numEmptyTasks];
    numEmptyTaskPrecs = new int[numEmptyTasks];
    numEmptyTaskAdds = new int[numEmptyTasks];
    emptyTaskPrecs = new int*[numEmptyTasks];
    emptyTaskAdds = new int*[numEmptyTasks];
    for (int i = 0; i < pgb; i++){
      numEmptyTaskPrecs[i] = pgb;
      numEmptyTaskAdds[i] = pgb + 1;
      emptyTaskPrecs[i] = new int[pgb];
      emptyTaskAdds[i] = new int[pgb + 1];
      emptyTaskAdds[i][pgb] = firstIndexTrans[firstStackIndex + i];
      for (int k = 0; k < pgb; k++){
        if (k < i){
          emptyTaskPrecs[i][k] = firstIndexTrans[firstConstraintIndex + k * (pgb - 1) + i - 1];
          emptyTaskAdds[i][k] = firstIndexTrans[firstConstraintIndex + i * (pgb - 1) + k];
        }
        else if (i == k){
          emptyTaskPrecs[i][k] = firstIndexTrans[firstTaskIndex + i] + 1;
          emptyTaskAdds[i][k] = firstIndexTrans[firstTaskIndex + i];
        }
        else {
          emptyTaskPrecs[i][k] = firstIndexTrans[firstConstraintIndex + k * (pgb - 1) + i];
          emptyTaskAdds[i][k] = firstIndexTrans[firstConstraintIndex + i * (pgb - 1) + k - 1];
        }
      }
      emptyTaskNames[i] = string("emptyTask(head ") + to_string(i) + ')';
    }
    
    // transformed actions and methods
    methodIndexes = new int[numMethods + 1];
    methodIndexes[0] = 0;
    for (int i = 1; i < numMethods + 1; i++){     
      int off = 1;
      if (hasNoLastTask[i - 1]){
        off = 0;
      }
      int m = bin(pgb - 1, numSubTasks[i - 1] - off);
      if (m < 1){
        m = 1;
      }
      if (m == INT_MAX){
        return -1;
      }
      if (numSubTasks[i - 1] <= 1){
        m = 1;
      }
      methodIndexes[i] = methodIndexes[i - 1] + m;
    }
    
    numMethodsTrans = methodIndexes[numMethods];
    numMethodsTrans *= pgb;
    numActionsTrans = numActions * pgb + numMethodsTrans;
    firstMethodIndex = numActions * pgb;
    actionCostsTrans = new int[numActionsTrans];
    invalidTransActions = new bool[numActionsTrans];
    for (int i = 0; i < numActionsTrans; i++) {
      invalidTransActions[i] = false;
    }
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        actionCostsTrans[i * pgb + j] = actionCosts[i];
      }
    }
    for (int i = firstMethodIndex; i < numActionsTrans; i++) {
      actionCostsTrans[i] = 0;
    }
    
    // transformed actions
    numPrecsTrans = new int[numActionsTrans];
    numAddsTrans = new int[numActionsTrans];
    numDelsTrans = new int[numActionsTrans];
    precListsTrans = new int*[numActionsTrans];
    addListsTrans = new int*[numActionsTrans];
    actionNamesTrans = new string[numActionsTrans];

    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        int index = i * pgb + j;
        numPrecsTrans[index] = numPrecs[i] + pgb;
        numAddsTrans[index] = numAdds[i] + pgb + 1;
        precListsTrans[index] = new int[numPrecsTrans[index]];
        addListsTrans[index] = new int[numAddsTrans[index]];
        for (int l = 0; l < numPrecs[i]; l++){
          precListsTrans[index][l] = precLists[i][l];
        }
        for (int l = 0; l < numAdds[i]; l++){
          addListsTrans[index][l] = addLists[i][l];
        }
        for (int l = 0; l < pgb; l++){
          if (l < j){
            precListsTrans[index][numPrecs[i] + l] = firstIndexTrans[firstConstraintIndex + l * (pgb - 1) + j - 1];
          }
          else if (j == l){
            precListsTrans[index][numPrecs[i] + l] = firstIndexTrans[firstTaskIndex + j] + 2 + i;
          }
          else {
            precListsTrans[index][numPrecs[i] + l] = firstIndexTrans[firstConstraintIndex + l * (pgb - 1) + j];
          }
        }
        actionNamesTrans[index] = "primitive(id[" + to_string(i) + "],head[" + to_string(j) + "]): " + taskNames[i];
        addListsTrans[index][numAdds[i]] = firstIndexTrans[firstTaskIndex + j];
        addListsTrans[index][numAddsTrans[index] - 1] = firstIndexTrans[firstStackIndex + j];
        for (int k = 0; k < pgb - 1; k++){
          addListsTrans[index][numAdds[i] + 1 + k] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + k];
        }
      }
    }

    taskToKill = new int[numMethods];
    for (int i = 0; i < numTasks; i++){
      for (int j = 0; j < numMethodsForTask[i]; j++){
        taskToKill[taskToMethods[i][j]] = i + 2;
      }
    }
    // transformed methods
    for (int i = 0; i < numMethods; i++) {
      int* subs = new int[numSubTasks[i]];
      for (int j = 0; j < pgb; j++){
        for (int k = 0; k < (methodIndexes[i + 1] - methodIndexes[i]); k++){
          int index = firstMethodIndex + methodIndexes[i] * pgb + j * (methodIndexes[i + 1] - methodIndexes[i]) + k;
          int inv = 0;
          if (numSubTasks[i] == 1){
            numAddsTrans[index] = 1;
            numPrecsTrans[index] = pgb;
          }
          else if (numSubTasks[i] == 0){
            numAddsTrans[index] = pgb + 1;
            numPrecsTrans[index] = pgb;
          }
          else {
            if (hasNoLastTask[i]){
              inv = 1;
              combination(subs, pgb - 1, numSubTasks[i], k);
              numAddsTrans[index] = numSubTasks[i] * 3 + 1 + numOrderings[i] / 2;
              numPrecsTrans[index] = numSubTasks[i] + 1 + pgb + subs[numSubTasks[i] - 1];
            }
            else {
              combination(subs, pgb - 1, numSubTasks[i] - 1, k);
              numAddsTrans[index] = numSubTasks[i] * 2 - 1 + numOrderings[i] / 2;
              numPrecsTrans[index] = numSubTasks[i] + pgb + subs[numSubTasks[i] - 2];
            }
          }
          precListsTrans[index] = new int[numPrecsTrans[index]];
          addListsTrans[index] = new int[numAddsTrans[index]];
          if (numSubTasks[i] + inv > pgb){
            invalidTransActions[index] = true;
            continue;
          }
          for (int m = 0; m < pgb; m++){
            if (m < j){
              precListsTrans[index][m] = firstIndexTrans[firstConstraintIndex + m * (pgb - 1) + j - 1];
            }
            else if (j == m){
              precListsTrans[index][m] = firstIndexTrans[firstTaskIndex + m] + taskToKill[i];
            }
            else {
              precListsTrans[index][m] = firstIndexTrans[firstConstraintIndex + m * (pgb - 1) + j];
            }
          }
          if (numSubTasks[i] == 0){
            addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j];
            addListsTrans[index][pgb] = firstIndexTrans[firstStackIndex + j];
            for (int m = 0; m < pgb - 1; m++){
              addListsTrans[index][1 + m] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + m];
            }
          }
          else if (numSubTasks[i] == 1){
            addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 2 + subTasksInOrder[i][0];
          }
          else {
            if (hasNoLastTask[i]){
              addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1;
              for (int m = 0; m < subs[numSubTasks[i] - 1] + 1; m++){
                int off = 0;
                if (m >= j){
                  off = 1;
                }
                precListsTrans[index][numSubTasks[i] + pgb + m] = firstIndexTrans[firstStackIndex + m + off] + 1;
              }
              for (int m = 0; m < numSubTasks[i]; m++){
                int off = 0;
                if (subs[m] >= j){
                  off = 1;
                  subs[m]++;
                }
                precListsTrans[index][m + pgb] = firstIndexTrans[firstTaskIndex + subs[m]];
                precListsTrans[index][numSubTasks[i] + pgb + subs[m] - off] = firstIndexTrans[firstStackIndex + subs[m]];
                addListsTrans[index][m + 1] = firstIndexTrans[firstTaskIndex + subs[m]] + 2 + subTasksInOrder[i][m];
                addListsTrans[index][m + numSubTasks[i] + 1] = firstIndexTrans[firstConstraintIndex + (subs[m]) * (pgb - 1) + j - 1 + off] + 1;
                addListsTrans[index][m + numSubTasks[i] * 2 + 1] = firstIndexTrans[firstStackIndex + (subs[m])] + 1;
              }
              for (int m = 0; m < numOrderings[i] / 2; m++){
                int first = 0;
                int free = ordering[i][2 * m];
                int constrained = ordering[i][2 * m + 1];
                if (subs[constrained] >= subs[free]){
                  first = 1;
                }
                addListsTrans[index][m + numSubTasks[i] * 3 + 1] = lastIndexTrans[firstConstraintIndex + subs[free] * (pgb - 1) + subs[constrained] - first];
              }
            }
            else {
              addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + subTasksInOrder[i][0] + 2;
              for (int m = 0; m < subs[numSubTasks[i] - 2] + 1; m++){
                int off = 0;
                if (m >= j){
                  off = 1;
                }
                precListsTrans[index][numSubTasks[i] + pgb - 1 + m] = firstIndexTrans[firstStackIndex + m + off] + 1;
              }
              for (int m = 0; m < numSubTasks[i] - 1; m++){
                int off = 0;
                if (subs[m] >= j){
                  off = 1;
                  subs[m]++;
                }
                precListsTrans[index][m + pgb] = firstIndexTrans[firstTaskIndex + subs[m]];
                precListsTrans[index][numSubTasks[i] + pgb - 1 + subs[m] - off] = firstIndexTrans[firstStackIndex + subs[m]];
                addListsTrans[index][m + 1] = firstIndexTrans[firstTaskIndex + subs[m]] + 2 + subTasksInOrder[i][m + 1];
                addListsTrans[index][m + numSubTasks[i]] = firstIndexTrans[firstStackIndex + subs[m]] + 1;
              }
              for (int m = 0; m < numOrderings[i] / 2; m++){
                int first = 0;
                int free = ordering[i][2 * m] - 1;
                int constrained = ordering[i][2 * m + 1] - 1;
                if (constrained == -1){
                  if (j >= subs[free]){
                    first = 1;
                  }
                  addListsTrans[index][m + numSubTasks[i] * 2 - 1] = lastIndexTrans[firstConstraintIndex + subs[free] * (pgb - 1) + j - first];
                }
                else {
                  if (subs[constrained] >= subs[free]){
                    first = 1;
                  }
                  addListsTrans[index][m + numSubTasks[i] * 2 - 1] = lastIndexTrans[firstConstraintIndex + subs[free] * (pgb - 1) + subs[constrained] - first];
                }
              }
            }
          }
          string name = "method(id[" + to_string(i) + "],head[" + to_string(j) + "],subtasks[";
          if (hasNoLastTask[i]){
            for (int l = 0; l < numSubTasks[i]; l++){
              name += to_string(subs[l]);
              if (l < numSubTasks[i] - 1){
                name += ",";
              }
            }
          }
          else {
            for (int l = 0; l < numSubTasks[i]; l++){
              if (l == 0){
                name += to_string(j);
              }
              else {
                name += to_string(subs[l - 1]);
              }
              if (l < numSubTasks[i] - 1){
                name += ",";
              }
            }
          }
          name += "]): " + methodNames[i];
          actionNamesTrans[index] = name;
        }
      }
      delete[] subs;
    }

    numInvalidTransActions = 0;
    for (int i = 0; i < numActionsTrans; i++) {
      if (invalidTransActions[i]){
        numInvalidTransActions++;
      }
    }
    return numActionsTrans;
  }

  void Model::combination(int* array, int n, int k, int i){
    for (int j = 0; j < k; j++){
      array[j] = j;
    }
    for (int j = 0; j < i; j++){
      int index = k - 1;
      int highest = n;
      while (index >= 0){
        array[index]++;
        if (array[index] < highest){
          break;
        }
        index--;
        highest--;
      }
      index++;
      while (index < k){
        array[index] = array[index - 1] + 1;
        index++;
      }
    }
  }
  
  int Model::bin(int n, int k){
   long b = 1;
   if (n < 0 || k < 0 || k > n){
     return -1;
   }
   if (k == 0 || k == n){
     return 1;
   }
   for (int i = 1; i <= k; i++){
      b = b * (n - i + 1) / i;
   }
   if (b > INT_MAX){
     return INT_MAX;
   }
   return int(b);
  }
 
  int Model::power(int n, int p){
    if (n == 0){
      return 0;
    }
    if (p == 0){
      return 1;
    }
    int i = 1;
    for (int j = 0; j < p; j++){
      i *= n;
    }
    return i;
  }
     
  int Model::htnPS(int numSeq, int* pgbList) {
    // number of variables: one head counter per sequence, the sequences themselves and constraints between sequences
    int pgb = 0;
    numVarsTrans = numVars + numSeq + 1 + numSeq * (numSeq - 1);
    for (int i = 0; i < numSeq; i++){
      numVarsTrans += pgbList[i];
      if (pgb < pgbList[i]){
        pgb = pgbList[i];
      }
    }
    // relevant indezes
    firstVarIndex = 0;
    headIndex = numVars + 1;
    firstConstraintIndex = numVars + numSeq + 1;
    firstTaskIndex = numVars + numSeq + 1 + numSeq * (numSeq - 1);
    varNamesTrans = new string[numVarsTrans];

    firstIndexTrans = new int[numVarsTrans];
    lastIndexTrans = new int[numVarsTrans];
    int* taskIndezes = new int[numSeq];
    
    
    for (int i = 0; i < numVars; i++){
      firstIndexTrans[firstVarIndex + i] = firstIndex[i];
      lastIndexTrans[firstVarIndex + i] = lastIndex[i];
    }
    // special var for first task
    firstIndexTrans[headIndex - 1] = lastIndexTrans[headIndex - 2] + 1;
    lastIndexTrans[headIndex - 1] = firstIndexTrans[headIndex - 1] + 1;
    for (int i = 0; i < numSeq; i++){
      firstIndexTrans[headIndex + i] = lastIndexTrans[headIndex - 1 + i] + 1;
      lastIndexTrans[headIndex + i] = firstIndexTrans[headIndex + i] + pgbList[i];
    }
    for (int i = 0; i < numSeq * (numSeq - 1); i++){
      firstIndexTrans[firstConstraintIndex + i] = lastIndexTrans[firstConstraintIndex + i - 1] + 1;
      lastIndexTrans[firstConstraintIndex + i] = firstIndexTrans[firstConstraintIndex + i] + 1;
    }
    int index = firstTaskIndex;
    for (int i = 0; i < numSeq; i++){
      taskIndezes[i] = index;
      for (int j = 0; j < pgbList[i]; j++){
        firstIndexTrans[index] = lastIndexTrans[index - 1] + 1;
        lastIndexTrans[index] = firstIndexTrans[index] + numTasks;
        index++;
      }
    }
    numStateBitsTrans = lastIndexTrans[numVarsTrans - 1] + 1;

    factStrsTrans = new string[numStateBitsTrans];
    
    // variable names
    varNamesTrans[headIndex - 1] = "var" + to_string(headIndex - 1) + ",Top";

    index = headIndex;
    for (int i = 0; i < numSeq; i++){
      varNamesTrans[index] = "var" + to_string(index) + ",sequence" + to_string(i);
      index++;
    }

    index = firstTaskIndex;
    for (int i = 0; i < numSeq; i++){
      for (int j = 0; j < pgbList[i]; j++){
        varNamesTrans[index] = "var" + to_string(index) + ",sequence" + to_string(i) + "head" + to_string(j);
        index++;
      }
    }
    index = firstConstraintIndex;
    for (int i = 0; i < numSeq; i++){
      for (int j = 0; j < numSeq - 1; j++){
        varNamesTrans[index] = "var" + to_string(index) + ",sequence" + to_string(i) + "constrains" + to_string(j);
        index++;
      }
    }
    for (int i = 0; i < numVars; i++){
      varNamesTrans[firstVarIndex + i] = varNames[i];
    }

    // state bit names
    for (int i = 0; i < numStateBits; i++){
      factStrsTrans[firstIndexTrans[firstVarIndex] + i] = factStrs[i];
    }
    factStrsTrans[firstIndexTrans[headIndex - 1]] = "+top[done]";
    factStrsTrans[firstIndexTrans[headIndex - 1] + 1] = "+top[active]";

    index = firstIndexTrans[headIndex];
    for (int i = 0; i < numSeq; i++){
      factStrsTrans[index] = "+sequence[done]";
      index++;
      for (int j = 0; j < pgbList[i]; j++){
        factStrsTrans[index] = string("+sequence[head")+ to_string(j) + ']';
        index++;
      }
    }
    index = firstIndexTrans[firstConstraintIndex];
    for (int i = 0; i < numSeq; i++){
      for (int j = 0; j < numSeq; j++){
        if (j == i){
          continue;
        }
        factStrsTrans[index] = string("+unconstrained[sequence") + to_string(i) + string(",sequence") + to_string(j) + ']';
        index++;
        factStrsTrans[index] = string("+constrained[sequence") + to_string(i) + string(",sequence") + to_string(j) + ']';
        index++;
      }
    }
    index = firstIndexTrans[firstTaskIndex];
    for (int i = 0; i < numSeq; i++){
      for (int j = 0; j < pgbList[i]; j++){
        factStrsTrans[index] = string("+task[sequence") + to_string(i)  + string(",head") + to_string(j) + string(",noTask]");
        index++;
        for (int k = 0; k < numTasks; k++){
          factStrsTrans[index] = string("+task[sequence") + to_string(i)  + string(",head") + to_string(j) + string(",task") + to_string(k) + ']';
          index++;
        }
      }
    }
    // Initial state
    s0SizeTrans = numVarsTrans - headIndex + 1;
    s0ListTrans = new int[s0SizeTrans];
    s0ListTrans[0] = firstIndexTrans[headIndex - 1] + 1;
    for (int i = 1; i < s0SizeTrans; i++){
      s0ListTrans[i] = firstIndexTrans[headIndex + i - 1];
    }

    // Goal state
    gSizeTrans = 1 + numSeq;
    gListTrans = new int[gSizeTrans];
    gListTrans[0] = firstIndexTrans[headIndex - 1];
    for (int i = 0; i < numSeq; i++){
      gListTrans[i + 1] = firstIndexTrans[headIndex + i];
    }
    
    // transformed actions and methods
    numActionsTrans = numActions * pgb * numSeq + numMethods * pgb * numSeq;
    firstMethodIndex = numActions * pgb * numSeq;

    actionCostsTrans = new int[numActionsTrans];
    invalidTransActions = new bool[numActionsTrans];
    for (int i = 0; i < numActionsTrans; i++) {
      invalidTransActions[i] = false;
    }
    numInvalidTransActions = 0;
    
    numPrecsTrans = new int[numActionsTrans];
    numAddsTrans = new int[numActionsTrans];
    precListsTrans = new int*[numActionsTrans];
    addListsTrans = new int*[numActionsTrans];

    actionNamesTrans = new string[numActionsTrans];

    // actions
    index = 0;
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        for (int k = 0; k < numSeq; k++){
          actionCostsTrans[index] = actionCosts[i];
          if (j >= pgbList[k]){
            invalidTransActions[index] = true;
            numInvalidTransActions++;
            index++;
            continue;
          }
          numPrecsTrans[index] = numPrecs[i] + 2 + numSeq - 1;
          numAddsTrans[index] = numAdds[i] + 2;
          if (j == 0){
            numAddsTrans[index] += numSeq - 1;
          }
          precListsTrans[index] = new int[numPrecsTrans[index]];
          addListsTrans[index] = new int[numAddsTrans[index]];
          for (int l = 0; l < numPrecs[i]; l++){
            precListsTrans[index][l] = precLists[i][l];
          }
          for (int l = 0; l < numAdds[i]; l++){
            addListsTrans[index][l] = addLists[i][l];
          }
          for (int l = 0; l < numSeq - 1; l++){
            int off = 0;
            if (l >= k){
              off = 1;
            }
            precListsTrans[index][numPrecs[i] + 2 + l] = firstIndexTrans[firstConstraintIndex + (l + off) * (numSeq - 1) + k - 1 + off];
            if (j == 0){
              addListsTrans[index][numAdds[i] + 2 + l] = firstIndexTrans[firstConstraintIndex + (numSeq - 1) * k + l];
            }
          }
          precListsTrans[index][numPrecs[i]] = firstIndexTrans[headIndex + k] + 1 + j;
          precListsTrans[index][numPrecs[i] + 1] = firstIndexTrans[taskIndezes[k] + j] + 1 + i;
          addListsTrans[index][numAdds[i]] = firstIndexTrans[headIndex + k] + j;
          addListsTrans[index][numAdds[i] + 1] = firstIndexTrans[taskIndezes[k] + j];

          actionNamesTrans[index] = "primitive(id[" + to_string(i)  + "],head[" + to_string(k * pgb + j) + "]): " + taskNames[i];
          index++;
        }
      }
    }
    
    taskToKill = new int[numMethods];
    for (int i = 0; i < numTasks; i++){
      for (int j = 0; j < numMethodsForTask[i]; j++){
        taskToKill[taskToMethods[i][j]] = i + 1;
      }
    }

    // methods
    index = firstMethodIndex;
    for (int i = 0; i < numMethods; i++) {
      for (int j = 0; j < pgb; j++){
        for (int k = 0; k < numSeq; k++){
          actionCostsTrans[index] = 0;
          if (numSubTasks[i] + j > pgbList[k] && !(taskToKill[i] == initialTask + 1)){
            invalidTransActions[index] = true;
            numInvalidTransActions++;
            index++;
            continue;
          }
          numPrecsTrans[index] = 2 + numSeq - 1;
          if (numSubTasks[i] == 0){
            numAddsTrans[index] = 2;
            if (j == 0){
              numAddsTrans[index] += numSeq - 1;
            }
          }
          else if (numSubTasks[i] == 1){
            numAddsTrans[index] = 1;
          }
          else {
            numAddsTrans[index] = numSubTasks[i] + 1;
          }
          // one top method
          if (taskToKill[i] == initialTask + 1){
            if (j == 0 && k == 0){
              numPrecsTrans[index] = 1;
              numAddsTrans[index] = numSeq * (numSeq + 1) + 1;
              precListsTrans[index] = new int[numPrecsTrans[index]];
              addListsTrans[index] = new int[numAddsTrans[index]];
              precListsTrans[index][0] = firstIndexTrans[headIndex - 1] + 1;
              addListsTrans[index][0] = firstIndexTrans[headIndex - 1];
              for (int l = 0; l < numSeq; l++){
                addListsTrans[index][1 + l] = firstIndexTrans[headIndex + l] + 1;
                addListsTrans[index][1 + l + numSeq] = firstIndexTrans[taskIndezes[l]] + subTasksInOrder[i][l] + 1;
                for (int m = 0; m < numSeq - 1; m++){
                  addListsTrans[index][1 + 2 * numSeq + l * (numSeq - 1) + m] = firstIndexTrans[firstConstraintIndex + l * (numSeq - 1) + m];
                }
              }
              for (int l = 0; l < numOrderings[i] / 2; l++){
                int off = 0;
                if (ordering[i][2 * l] < ordering[i][2 * l + 1]){
                  off = 1;
                }
                int order = ordering[i][2 * l] * (numSeq - 1) + ordering[i][2 * l + 1] - off;
                addListsTrans[index][1 + 2 * numSeq + order] = firstIndexTrans[firstConstraintIndex + order] + 1;
              }
            }
            else {
              invalidTransActions[index] = true;
              numInvalidTransActions++;
            }
            actionNamesTrans[index] = "method(id[" + to_string(i) + "],head[" + to_string(k * pgb + j);
            actionNamesTrans[index] += "],subtasks[" + to_string(0);
            for (int l = 1; l < numSubTasks[i]; l++){
              actionNamesTrans[index] += ',' + to_string(l * pgb);
            }
            actionNamesTrans[index] += "]): " + methodNames[i];
            index++;
            continue;
          }
          precListsTrans[index] = new int[numPrecsTrans[index]];
          addListsTrans[index] = new int[numAddsTrans[index]];

          precListsTrans[index][0] = firstIndexTrans[headIndex + k] + 1 + j;
          precListsTrans[index][1] = firstIndexTrans[taskIndezes[k] + j] + taskToKill[i];
          for (int l = 0; l < numSeq - 1; l++){
            int off = 0;
            if (l >= k){
              off = 1;
            }
            precListsTrans[index][2 + l] = firstIndexTrans[firstConstraintIndex + (l + off) * (numSeq - 1) + k - 1 + off];
            if (j == 0 && numSubTasks[i] == 0){
              addListsTrans[index][2 + l] = firstIndexTrans[firstConstraintIndex + (numSeq - 1) * k + l];
            }
          }
          if (numSubTasks[i] == 0){
            addListsTrans[index][0] = firstIndexTrans[headIndex] + j;
            addListsTrans[index][1] = firstIndexTrans[taskIndezes[k] + j];
          }
          else if (numSubTasks[i] == 1){
            addListsTrans[index][0] = firstIndexTrans[taskIndezes[k] + j] + 1 + subTasksInOrder[i][0];
          }
          else {
            for (int l = 0; l < numSubTasks[i]; l++){
              addListsTrans[index][l] = firstIndexTrans[taskIndezes[k] + j + l] + 1 + subTasksInOrder[i][l];
            }
            addListsTrans[index][numAddsTrans[index] - 1] = firstIndexTrans[headIndex + k] + j + numSubTasks[i];
          }

          actionNamesTrans[index] = "method(id[" + to_string(i) + "],head[" + to_string(k * pgb + j);
          if (numSubTasks[i] > 0){
            actionNamesTrans[index] += "],subtasks[" + to_string(k * pgb + j);
            for (int l = 1; l < numSubTasks[i]; l++){
              actionNamesTrans[index] += ',' + to_string(k * pgb + j + l);
            }
          }
          actionNamesTrans[index] += "]): " + methodNames[i];
          
          index++;
        }
      }
    }
    delete[] taskIndezes;
	return numActionsTrans;
  }

  void Model::tohtnToStrips(int pgb) {
    // number of translated variables
    numVarsTrans = numVars + 1 + pgb;
    // indizes for variables
    firstVarIndex = 0;
    headIndex = numVars;
    firstTaskIndex = numVars + 1;
    varNamesTrans = new string[numVarsTrans];
    numInvalidTransActions = 0;


    // indizes for names
    firstIndexTrans = new int[numVarsTrans];
    lastIndexTrans = new int[numVarsTrans];

    for (int i = 0; i < numVars; i++){
      firstIndexTrans[firstVarIndex+ i] = firstIndex[i];
      lastIndexTrans[firstVarIndex+i] = lastIndex[i];
    }

    firstIndexTrans[headIndex] = lastIndexTrans[headIndex - 1] + 1;
    lastIndexTrans[headIndex] = firstIndexTrans[headIndex] + pgb;

    for (int i = 0; i < pgb; i++){
      firstIndexTrans[firstTaskIndex + i] = lastIndexTrans[firstTaskIndex + i - 1] + 1;
      lastIndexTrans[firstTaskIndex + i] = firstIndexTrans[firstTaskIndex + i] + numTasks;
    	cout << firstIndexTrans[firstTaskIndex + i] << " " << lastIndexTrans[firstTaskIndex + i] << endl;
	}

    numStateBitsTrans = lastIndexTrans[numVarsTrans - 1] + 1;

    factStrsTrans = new string[numStateBitsTrans];


    varNamesTrans[headIndex] = "varHead";
    for (int i = firstTaskIndex; i < numVarsTrans ; i++){
      varNamesTrans[i] = "varTasksHead" + to_string(i);
    }
    for (int i = 0; i < numVars; i++){
      varNamesTrans[firstVarIndex + i] = varNames[i];
    }


    for (int i = 0; i < numStateBits; i++){
      factStrsTrans[firstIndexTrans[firstVarIndex] + i] = factStrs[i];
    }
    factStrsTrans[firstIndexTrans[headIndex]] = "ppoint(head, finish)";
    for (int i = 0; i < pgb; i++){
      factStrsTrans[i + firstIndexTrans[headIndex] + 1] = string("ppoint(head, point")+ to_string(i) + ')';
    }
    for (int i = firstTaskIndex; i < numVarsTrans; i++){
      factStrsTrans[firstIndexTrans[i]] = string("ptask(point") + to_string(i-firstTaskIndex) + string(", noTask)")    ;
      for (int j = 0; j < lastIndexTrans[i] - firstIndexTrans[i]; j++){
        factStrsTrans[firstIndexTrans[i]+j + 1] = string("ptask(point") + to_string(i - firstTaskIndex) + string(", task") + to_string(j) + ')';
      }
    }

    // Initial state
    s0SizeTrans = numVarsTrans-headIndex;
    s0ListTrans = new int[s0SizeTrans];
    s0ListTrans[0] = firstIndexTrans[headIndex] + 1;
    s0ListTrans[1] = firstIndexTrans[firstTaskIndex] + initialTask + 1;
    for (int i = 1; i < s0SizeTrans - 1; i++){
      s0ListTrans[i + 1] = firstIndexTrans[firstTaskIndex + i];
    }

    // Goal state
    gSizeTrans = 1;
    gListTrans = new int[gSizeTrans];
    gListTrans[0] = firstIndexTrans[headIndex];

    // transformed actions and methods
    numActionsTrans = numActions * pgb + numMethods * pgb;
    firstMethodIndex = numActions * pgb;
    actionCostsTrans = new int[numActionsTrans];
    invalidTransActions = new bool[numActionsTrans];
    for (int i = 0; i < numActionsTrans; i++) {
      invalidTransActions[i] = false;
    }
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        actionCostsTrans[i * pgb + j] = actionCosts[i];
      }
    }
    for (int i = firstMethodIndex; i < numActionsTrans; i++) {
      actionCostsTrans[i] = 0;
    }

    // transformed actions
    numPrecsTrans = new int[numActionsTrans];
    numAddsTrans = new int[numActionsTrans];
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        numPrecsTrans[i * pgb + j] = numPrecs[i] + 2;
        numAddsTrans[i * pgb + j] = numAdds[i] + 2;
      }
    }

    for (int i = 0; i < numMethods; i++) {
      for (int j = 0; j < pgb; j++){
        if (numSubTasks[i] + j - firstNumTOPrimTasks[i] > pgb){
          invalidTransActions[firstMethodIndex + i * pgb + j] = true;
        }
        numPrecsTrans[firstMethodIndex + i * pgb + j] = 2;
        if (numSubTasks[i] - firstNumTOPrimTasks[i] == 0){
          numAddsTrans[firstMethodIndex + i * pgb + j] = 2;
        }
        else if (numSubTasks[i] - firstNumTOPrimTasks[i] == 1){
          numAddsTrans[firstMethodIndex + i * pgb + j] = 1;
        }
        else {
          numAddsTrans[firstMethodIndex + i * pgb + j] = numSubTasks[i] - firstNumTOPrimTasks[i] + 1;
        }
      }
    }

    // invalid Methods
    numInvalidTransActions = 0;
    for (int i = 0; i < numActionsTrans; i++) {
      if (invalidTransActions[i]){
        numInvalidTransActions++;
      }
    }

    //precs, dels, adds
    precListsTrans = new int*[numActionsTrans];
    for (int i = 0; i < numActionsTrans; i++) {
      precListsTrans[i] = new int[numPrecsTrans[i]];
    }
    addListsTrans = new int*[numActionsTrans];
    for (int i = 0; i < numActionsTrans; i++) {
      addListsTrans[i] = new int[numAddsTrans[i]];
    }

    //precs, dels, adds for actions
    for (int i = 0; i < numActions; i++) {
      for (int j = 0; j < pgb; j++){
        for (int k = 0; k < numPrecs[i]; k++){
          precListsTrans[i * pgb + j][k] = precLists[i][k];
        }
        precListsTrans[i * pgb + j][numPrecsTrans[i * pgb + j] - 1] = firstIndexTrans[headIndex] + 1 + j;
        precListsTrans[i * pgb + j][numPrecsTrans[i * pgb + j] - 2] = firstIndexTrans[firstTaskIndex + j] + 1 + i;
        for (int k = 0; k < numAdds[i]; k++){
          addListsTrans[i * pgb + j][k] = addLists[i][k];
        }
        addListsTrans[i * pgb + j][numAddsTrans[i * pgb + j] - 1] = firstIndexTrans[headIndex] + j;
        addListsTrans[i * pgb + j][numAddsTrans[i * pgb + j] - 2] = firstIndexTrans[firstTaskIndex + j];
      }
    }
    taskToKill = new int[numMethods];
    for (int i = 0; i < numTasks; i++){
      for (int j =0; j < numMethodsForTask[i]; j++){
        taskToKill[taskToMethods[i][j]] = i + 1;
      }
    }
    //precs, dels, adds for methods
    for (int i = 0; i < numMethods; i++) {
        for (int j = 0; j < pgb; j++){
            if (invalidTransActions[firstMethodIndex + i * pgb + j]){
                continue;
            }
            precListsTrans[firstMethodIndex + i * pgb + j][0] = firstIndexTrans[headIndex] + 1 + j;
            precListsTrans[firstMethodIndex + i * pgb + j][1] = firstIndexTrans[firstTaskIndex + j] + taskToKill[i];
            if (numSubTasks[i] - firstNumTOPrimTasks[i] == 0){
              addListsTrans[firstMethodIndex + i * pgb + j][1] = firstIndexTrans[firstTaskIndex + j];
              addListsTrans[firstMethodIndex + i * pgb + j][0] = firstIndexTrans[headIndex] + j;
              continue;
            }
            if (numSubTasks[i] - firstNumTOPrimTasks[i] == 1){
              addListsTrans[firstMethodIndex + i * pgb + j][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
              continue;
            }
            for (int k = 0; k < numSubTasks[i] - firstNumTOPrimTasks[i]; k++){
              addListsTrans[firstMethodIndex + i * pgb + j][k] = firstIndexTrans[firstTaskIndex + j + k] + 1 + subTasksInOrder[i][k];
            }
            addListsTrans[firstMethodIndex + i * pgb + j][numAddsTrans[firstMethodIndex + i * pgb + j] - 1] = firstIndexTrans[headIndex] + j + numSubTasks[i] - firstNumTOPrimTasks[i];
        }
    }
    // names
    actionNamesTrans = new string[numActionsTrans];
    for (int i = 0; i < numActions; i++) {
        for (int j = 0; j < pgb; j++){
            actionNamesTrans[i * pgb + j] = "primitive(id[" + to_string(i)  + "],head[" + to_string(j) + "]): " + taskNames[i];
        }
    }
    for (int i = 0; i < numMethods; i++) {
        for (int j = 0; j < pgb; j++){
            actionNamesTrans[firstMethodIndex + i * pgb + j] = "method(id[" + to_string(i) + "],head[" + to_string(j);
            if (numSubTasks[i] - firstNumTOPrimTasks[i] > 0){
              actionNamesTrans[firstMethodIndex + i * pgb + j] += "],subtasks[" + to_string(j);
              for (int k = 1; k < numSubTasks[i] - firstNumTOPrimTasks[i]; k++){
                actionNamesTrans[firstMethodIndex + i * pgb + j] += ',' + to_string(j + k);
              }
            }
            if (firstNumTOPrimTasks[i] > 0){
              actionNamesTrans[firstMethodIndex + i * pgb + j] += "],firstPrim[" + to_string(subTasksInOrder[i][numSubTasks[i] - 1]);
              for (int k = 1; k < firstNumTOPrimTasks[i]; k++){
                actionNamesTrans[firstMethodIndex + i * pgb + j] += ',' + to_string(subTasksInOrder[i][numSubTasks[i] - 1 - k]);
              }
            }
            actionNamesTrans[firstMethodIndex + i * pgb + j] += "]): " + methodNames[i];
        }
    }
}

  bool Model::parallelSequences(){
    return (numMethodsForTask[initialTask] == 1);
  }
  
  void Model::writeToFastDown(string sasName, int problemType, int pgb) {
    ofstream sasfile;
    sasfile.open (sasName);
    // version
    sasfile << "begin_version" << endl;
    sasfile << "3" << endl;
    sasfile << "end_version" << endl;
    sasfile <<  endl;
    // metric
    sasfile << "begin_metric" << endl;
    sasfile << "0" << endl;
    sasfile << "end_metric" << endl;
    sasfile <<  endl;

    int** convertMutexVars = new int*[this->numStateBitsTrans];
    for (int i = 0; i < this->numStateBitsTrans; i++) {
        convertMutexVars[i] = new int[2];
    }

    // variable section
    sasfile <<  this->numVarsTrans << endl;
    for (int i = 0; i < this->numVarsTrans; i++){
      int first_index = this->firstIndexTrans[i];
      int last_index = this->lastIndexTrans[i];
      sasfile << "begin_variable" << endl;
      sasfile << this->varNamesTrans[i] << endl;
      sasfile << -1 << endl;
      sasfile << last_index - first_index + 1 << endl;
      for (int j = 0; j < last_index - first_index + 1; j++){
        string ns = this->factStrsTrans[first_index + j];
        convertMutexVars[first_index + j][0] = i;
        convertMutexVars[first_index + j][1] = j;
		if (ns == "none-of-them")
			sasfile << "<none of those>" << endl;
		else
			sasfile << "Atom " << ns << endl;
      }
  
      sasfile << "end_variable" << endl;
    }
    sasfile <<  endl;
    // mutex section
    sasfile <<  this->numMutexes + this->numStrictMutexes << endl;
    for (int i = 0; i < this->numStrictMutexes; i++){
      int size = this->strictMutexesSize[i];
      sasfile << "begin_mutex_group" << endl;
      sasfile << size << endl;
      for (int j = 0; j < size; j++){
        sasfile << convertMutexVars[this->strictMutexes[i][j]][0] << " ";
        sasfile << convertMutexVars[this->strictMutexes[i][j]][1] << endl;
      }
      sasfile << "end_mutex_group" << endl;
    }
    for (int i = 0; i < this->numMutexes; i++){
      int size = this->mutexesSize[i];
      sasfile << "begin_mutex_group" << endl;
      sasfile << size << endl;
      for (int j = 0; j < size; j++){
        sasfile << convertMutexVars[this->mutexes[i][j]][0] << " ";
        sasfile << convertMutexVars[this->mutexes[i][j]][1] << endl;
      }
      sasfile << "end_mutex_group" << endl;
    }
    sasfile <<  endl;
    // initial state
    sasfile << "begin_state" << endl;
    for (int i = 0; i < this->s0Size; i++) {
      sasfile << convertMutexVars[this->s0List[i]][1] << endl;
    }
    for (int i = 0; i < this->s0SizeTrans; i++) {
      sasfile << convertMutexVars[this->s0ListTrans[i]][1] << endl;
    }
    sasfile << "end_state" << endl;
    sasfile <<  endl;
    // goal state -> different for htn_to_strips
    sasfile << "begin_goal" << endl;
    sasfile << gSizeTrans + gSize << endl;
    for (int i = 0; i < gSize; i++){
      sasfile << convertMutexVars[gList[i]][0] << " " << convertMutexVars[gList[i]][1] << endl;
    }
    for (int i = 0; i < gSizeTrans; i++){
      sasfile << convertMutexVars[gListTrans[i]][0] << " " << convertMutexVars[gListTrans[i]][1] << endl;
    }
    sasfile << "end_goal" << endl;
    sasfile <<  endl;

    // operator section
    int a = this->numActionsTrans + numEmptyTasks;
    sasfile << a - this->numInvalidTransActions << endl;
    for (int i = 0; i < numEmptyTasks; i++) {
      sasfile << "begin_operator" << endl;
      // name
      string tn = emptyTaskNames[i];

      sasfile << tn << endl;
      // action
      int numP = this->numEmptyTaskPrecs[i];
      int** precs = new int*[numP];
      int n = numP;
      for (int j = 0; j < numP; j++){
        precs[j] = new int[3];
      }
      for (int j = 0; j < numP; j++) {
        precs[j][0] = convertMutexVars[this->emptyTaskPrecs[i][j]][0];
        precs[j][1] = convertMutexVars[this->emptyTaskPrecs[i][j]][1];
        precs[j][2] = 0;
      }
      int numA = this->numEmptyTaskAdds[i];
      for (int j = 0; j < numP; j++) {
        for (int k = 0; k < numA; k++) {
          if (precs[j][0] == convertMutexVars[emptyTaskAdds[i][k]][0]){
            precs[j][2] += 2;
            if (precs[j][2] == 2){
              n--;
            }
          }
        }
      }
      sasfile << n << endl;
      for (int j = 0; j < numP; j++) {
        if (precs[j][2] == 0){
          sasfile << precs[j][0] << " ";
          sasfile << precs[j][1] << endl;
        }
      }
      int condEff = 0;
      sasfile << numA + condEff << endl;
      for (int j = 0; j < numA; j++) {
        int cost = 0;
        int var0 = convertMutexVars[emptyTaskAdds[i][j]][0];
        int var1 = convertMutexVars[emptyTaskAdds[i][j]][1];
        int preco = -1;
        for (int k = 0; k < numP; k++){
          if (precs[k][0] == var0){
            preco = precs[k][1];
          }
        }
        sasfile << cost << " " << var0 << " " << preco << " " << var1 << endl;
      }
      sasfile << this->actionCostsTrans[i] << endl;
      sasfile << "end_operator" << endl;
      for (int j = 0; j < numP; j++){
        delete[] precs[j];
      }
      delete[] precs;
    }
    for (int i = 0; i < numActionsTrans; i++) {
      if (invalidTransActions[i]){
          continue;
      }
      // name
      string tn = this->actionNamesTrans[i];


      bool primTasks = false;
      size_t primTasksIndex = tn.find("firstPrim");
      string primTaskString = "";
      if (primTasksIndex < tn.length()){
        primTasks = true;
        primTaskString = tn.substr(primTasksIndex + 9, tn.find(": ") - 1 - primTasksIndex - 9);
      }
      int primP = 0;
      int primA = 0;
      int * primPList = new int[numStateBits];
      int * primAList = new int[numStateBits];
      bool inappliccable = false;
      if (primTasks){
        int * pNum = new int[2];
        int err = calculatePrecsAndAdds(pNum, primPList, primAList, primTaskString, convertMutexVars);
        primP = pNum[0];
        primA = pNum[1];
        if (err < 0){
          cerr << endl << "tasks for Method:" << endl << tn << endl << "not applicable in this order: " << primTaskString << endl;
          cerr << "not printing method!" << endl;
          inappliccable = true;
          delete[] primPList;
          delete[] primAList;
        }
        delete[] pNum;
      }

      sasfile << "begin_operator" << endl;
      sasfile << tn << endl;
      if (inappliccable){
        sasfile << 0 << endl;
        sasfile << 0 << endl;
        sasfile << 1 << endl;
        sasfile << "end_operator" << endl;
        continue;
      }
      // action
      int numP = this->numPrecsTrans[i] + primP;
      int** precs = new int*[numP];
      int n = numP;
      for (int j = 0; j < numP; j++){
        precs[j] = new int[3];
      }
      for (int j = 0; j < numP - primP; j++) {
        precs[j][0] = convertMutexVars[this->precListsTrans[i][j]][0];
        precs[j][1] = convertMutexVars[this->precListsTrans[i][j]][1];
        precs[j][2] = 0;
      }
      for (int j = 0; j < primP; j++) {
        precs[j + numP - primP][0] = convertMutexVars[primPList[j]][0];
        precs[j + numP - primP][1] = convertMutexVars[primPList[j]][1];
        precs[j + numP - primP][2] = 0;
      }
      int numA = this->numAddsTrans[i] + primA;
      for (int j = 0; j < numP; j++) {
        for (int k = 0; k < numA; k++) {
          if (k < numA - primA){
            if (precs[j][0] == convertMutexVars[addListsTrans[i][k]][0]){
              precs[j][2] += 2;
              if (precs[j][2] == 2){
                n--;
              }
            }
          }
          else {
            if (precs[j][0] == convertMutexVars[primAList[k - numA + primA]][0]){
              precs[j][2] += 2;
              if (precs[j][2] == 2){
                n--;
              }
            }
          }
        }
      }
      sasfile << n << endl;
      for (int j = 0; j < numP; j++) {
        if (precs[j][2] == 0){
          sasfile << precs[j][0] << " ";
          sasfile << precs[j][1] << endl;
        }
      }
      int condEff = 0;
      if (problemType == 1 || problemType == 4){
        condEff = numConditionalEffectsTrans[i];
      }
      sasfile << numA + condEff << endl;
      for (int j = 0; j < numA - primA; j++) {
        int cost = 0;
        int var0 = convertMutexVars[addListsTrans[i][j]][0];
        int var1 = convertMutexVars[addListsTrans[i][j]][1];
        int preco = -1;
        for (int k = 0; k < numP; k++){
          if (precs[k][0] == var0){
            preco = precs[k][1];
          }
        }
        sasfile << cost << " " << var0 << " " << preco << " " << var1 << endl;
      }
      for (int j = 0; j < primA; j++) {
        int cost = 0;
        int var0 = convertMutexVars[primAList[j]][0];
        int var1 = convertMutexVars[primAList[j]][1];
        int preco = -1;
        for (int k = 0; k < numP; k++){
          if (precs[k][0] == var0){
            preco = precs[k][1];
          }
        }
        sasfile << cost << " " << var0 << " " << preco << " " << var1 << endl;
      }
      for (int j = 0; j < condEff; j++) {
        int ncon = numEffectConditionsTrans[i][j];
        int* cons = new int[ncon * 2];
        for (int k = 0; k < ncon; k++){
          cons[2 * k] = convertMutexVars[effectConditionsTrans[i][j][k]][0];
          cons[2 * k + 1] = convertMutexVars[effectConditionsTrans[i][j][k]][1];
        }
        int var0 = convertMutexVars[effectsTrans[i][j]][0];
        int var1 = convertMutexVars[effectsTrans[i][j]][1];
        int preco = -1;
        sasfile << ncon;
        for (int k = 0; k < ncon; k++){
          sasfile << " " << cons[2 * k] << " " << cons[2 * k + 1];
        }
        sasfile << " " << var0 << " " << preco << " " << var1 << endl;
        delete[] cons;
      }

      sasfile << this->actionCostsTrans[i] << endl;
      sasfile << "end_operator" << endl;
      for (int j = 0; j < numP; j++){
        delete[] precs[j];
      }
      delete[] precs;
      delete[] primPList;
      delete[] primAList;
    }
    // axiom section
    sasfile << 0 << endl;
    sasfile.close();
    for (int i = 0; i < numStateBitsTrans; i++){
      delete[] convertMutexVars[i];
    }
    delete[] convertMutexVars;
  }
  int Model::minProgressionBound(){
    int i = minImpliedPGB[initialTask];
    if (i < 1){
      return 1;
    }
    return i;
  }
  int Model::maxProgressionBound(){
    return 5000000;
  }
  
  int Model::calculatePrecsAndAdds(int* s, int* p, int* a, string tasks, int** conv){
    int index = tasks.find(",");
    if (index > tasks.length()){
      int task = stoi(tasks.substr(1, tasks.length() - 2));
      s[0] = numPrecs[task];
      s[1] = numAdds[task];
      for (int i = 0; i < s[0]; i++){
        p[i] = precLists[task][i];
      }
      for (int i = 0; i < s[1]; i++){
        a[i] = addLists[task][i];
      }
      return 0;
    }
    int nT = 1;
    while (index < tasks.length()){
      nT++;
      index = tasks.find(",", index + 1);
    }
    int * task = new int[nT];
    index = tasks.find(",");
    task[0] = stoi(tasks.substr(1, index - 1));
    for (int i = 1; i < nT - 1; i++){
      index++;
      int j = tasks.find(",", index);
      task[i] = stoi(tasks.substr(index, j - index));
      index = j;
    }
    task[nT - 1] = stoi(tasks.substr(index + 1, tasks.length() - index - 2));
    int** varChange = new int*[numVars];
    for (int i = 0; i < numVars; i++){
      varChange[i] = new int[2 * nT];
      for (int j = 0; j < 2 * nT; j++){
        varChange[i][j] = -1;
      }
    }
    for (int i = 0; i < nT; i++){
      for (int j = 0; j < numPrecs[task[i]]; j++){
        varChange[conv[precLists[task[i]][j]][0]][2 * i] = precLists[task[i]][j];
      }
      for (int j = 0; j < numAdds[task[i]]; j++){
        varChange[conv[addLists[task[i]][j]][0]][2 * i + 1] = addLists[task[i]][j];
      }
    }
    s[0] = 0;
    s[1] = 0;
    for (int i = 0; i < numVars; i++){
      int start = -1;
      int end = -1;
      for (int j = 0; j < nT; j++){
        if (end == -1){
          if (start == -1){
            start = varChange[i][2 * j];
          }
        }
        else if (varChange[i][2 * j] != -1 && end != varChange[i][2 * j]){
          cerr << endl << "variable: " << i << ", value old: " << end << ", value should be: " << varChange[i][2 * j] << endl;
          for (int j = 0; j < numVars; j++){
            delete[] varChange[j];
          }
          delete[] varChange;
          delete[] task;
          return -1;
        }
        if (varChange[i][2 * j + 1] != -1){
          end = varChange[i][2 * j + 1];
        }
      }
      if (start != -1){
        p[s[0]] = start;
        s[0]++;
      }
      if (end != -1){
        a[s[1]] = end;
        s[1]++;
      }
    }
    for (int j = 0; j < numVars; j++){
      delete[] varChange[j];
    }
    delete[] varChange;
    delete[] task;
    return 0;
  }
  
  void Model::planToHddl(string infile, string outfile) {
    ostream * _fout = &cout;
    ofstream * of = nullptr;
    if (outfile != "stdout"){
      of = new ofstream(outfile);
      if (!of->is_open()){
        cout << "I can't open " << outfile << "!" << endl;
        exit(1);
      }
      _fout = of;
    }
      
    ostream & fout = *_fout;
  
    istream* fin;
    ifstream* fileInput = new ifstream(infile);
    if (!fileInput->good()) {
        std::cerr << "Unable to open input file " << infile << ": " << strerror (errno) << std::endl;
        exit(1);
    }

    fin = fileInput;
    
    string line;
    int linecount = 0;
    while (fin->good()){
      linecount++;
      getline(*fin, line);
    }

    fileInput = new ifstream(infile);

    fin = fileInput;
    string* plan = new string[linecount];
        
    int methNumMax = 0;
    int primNumMax = 0;

    for (int i = 0; i < linecount; i++){
      getline(*fin, plan[i]);
      if (plan[i].size() > 9 && string("primitive").compare(plan[i].substr(1, 9)) == 0){
        primNumMax++;
      }
      if (plan[i].size() > 6 && string("method").compare(plan[i].substr(1, 6)) == 0){
        methNumMax++;
        int index = plan[i].find("firstPrim");
        if (index < plan[i].length()){
          primNumMax += firstNumTOPrimTasks[stoi(plan[i].substr(plan[i].find("id") + 3, plan[i].find("]", plan[i].find("id"), 1) - plan[i].find("id") - 3))];
        }
      }
    }

    fileInput->close();
    string* primitives = new string[primNumMax];
    string* methods = new string[methNumMax];
    int* methIndex = new int[methNumMax];
    int* primIndex = new int[primNumMax];
    int** subHeads = new int*[methNumMax];
    bool* hasPrimIndezes = new bool[methNumMax];
    int** firstPrimIndezes = new int*[methNumMax];
    int* order = new int[linecount];
    int* heads = new int[linecount];
    fout << "==>" << endl;
    int primNum = 0;
    int methNum = 0;
    for (int i = 0; i < linecount; i++){
      order[i] = -1;
      if (plan[i].size() > 9 && string("primitive").compare(plan[i].substr(1, 9)) == 0){
        primitives[primNum] = plan[i].substr(plan[i].find(": ") + 2, plan[i].length() - plan[i].find(": ") - 3);
        primIndex[primNum] = stoi(plan[i].substr(plan[i].find("id") + 3, plan[i].find("]", plan[i].find("id"), 1) - plan[i].find("id") - 3));
        order[i] = primNum;
        heads[i] = stoi(plan[i].substr(plan[i].find("head") + 5, plan[i].find("]", plan[i].find("head"), 1) - plan[i].find("head") - 5));
        fout << primNum << " " << primitives[primNum] << endl;
        primNum++;
      }
      else if (plan[i].size() > 6 && string("method").compare(plan[i].substr(1, 6)) == 0){
        methods[methNum] = plan[i].substr(plan[i].find(": ") + 2, plan[i].length() - plan[i].find(": ") - 3);
        methIndex[methNum] = stoi(plan[i].substr(plan[i].find("id") + 3, plan[i].find("]", plan[i].find("id"), 1) - plan[i].find("id") - 3));
        order[i] = methNum + primNumMax;
        heads[i] = stoi(plan[i].substr(plan[i].find("head") + 5, plan[i].find("]", plan[i].find("head"), 1) - plan[i].find("head") - 5));
        subHeads[methNum] = new int[numSubTasks[methIndex[methNum]]];
        int index = plan[i].find("firstPrim");
        hasPrimIndezes[methNum] = false;
        if (index < plan[i].length()){
          hasPrimIndezes[methNum] = true;
          firstPrimIndezes[methNum] = new int[firstNumTOPrimTasks[methIndex[methNum]]];
          string s = plan[i].substr(index + 10, plan[i].find("]", index, 1) - index - 10);
          for (int j = 0; j < firstNumTOPrimTasks[methIndex[methNum]]; j++){
            int f = s.find(",");
            if (f < s.length()){
              firstPrimIndezes[methNum][j] = stoi(s.substr(0, f));
              s = s.substr(f + 1, s.length());
            }
            else {
              firstPrimIndezes[methNum][j] = stoi(s);
            }
            primitives[primNum] = taskNames[firstPrimIndezes[methNum][j]];
            primIndex[primNum] = firstPrimIndezes[methNum][j];
            fout << primNum << " " << primitives[primNum] << endl;
            firstPrimIndezes[methNum][j] = primNum;
            primNum++;
          }
        }
        index = plan[i].find("subtasks");
        if (index < plan[i].length()){
          string s = plan[i].substr(index + 9, plan[i].find("]", index, 1) - index - 9);
          for (int j = 0; j < numSubTasks[methIndex[methNum]]; j++){
            int f = s.find(",");
            if (f < s.length()){
              subHeads[methNum][j] = stoi(s.substr(0, f));
              s = s.substr(f + 1, s.length());
            }
            else {
              subHeads[methNum][j] = stoi(s);
            }
          }
        }
        methNum++;
      }
      else if (plan[i].size() > 4 && string("move").compare(plan[i].substr(1, 4)) == 0){
        int from = stoi(plan[i].substr(plan[i].find("from") + 5, plan[i].find("]", plan[i].find("from"), 1) - plan[i].find("from") - 5));
        int to = stoi(plan[i].substr(plan[i].find("to") + 3, plan[i].find("]", plan[i].find("to"), 1) - plan[i].find("to") - 3));
        bool repl = false;
        for (int j = methNum - 1; j >= 0; j--){
          if (repl){
            break;
          }
          for (int k = 0; k < numSubTasks[methIndex[j]]; k++){
            if (subHeads[j][k] == from){
              subHeads[j][k] = to;
              repl = true;
              break;
            }
          }
        }
      }
    }
    fout << "root " << primNumMax << endl;
    for (int i = 0; i < methNumMax; i++){
      int m = methIndex[i];
      int o = 0;
      for (int j = 0; j < linecount; j++){
        if (order[j] == i + primNumMax){
          o = j;
          break;
        }
      }
      fout << order[o] << " " << taskNames[decomposedTask[m]] << " -> ";
      fout << methods[i] << " ";
      int fntopt = 0;
      if (hasPrimIndezes[i]){
        fntopt = firstNumTOPrimTasks[methIndex[i]];
        for (int j = 0; j < fntopt; j++){
          fout << firstPrimIndezes[i][j] << " ";
        }
      }
      for (int j = numSubTasks[m] - 1 - fntopt; j >= 0; j--){
        int index = -1;
        for (int k = o + 1; k < linecount - 1; k++){
          if (order[k] == -1){
            continue;
          }
          if (order[k] < primNumMax){
            if (primIndex[order[k]] == subTasksInOrder[m][j]){
              if (heads[k] != subHeads[i][j]){
                continue;
              }
              index = order[k];
              break;
            }
          }
          else {
            if (subTasksInOrder[m][j] == decomposedTask[methIndex[order[k]-primNumMax]]){
              if (heads[k] != subHeads[i][j]){
                continue;
              }
              index = order[k];
              break;
            }
          }
        }
        fout << index << " ";
      }
      fout << endl;
    }
    
    fout << "<==" << endl;
    if (of != nullptr) of->close();
 	}

}

/* namespace progression */
