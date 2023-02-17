#include "htnToSAS.h"

#include <fstream>
#include <cstring>
#include <list>
#include <sys/time.h>

using namespace std;


//// mathematical helper functions
void HTNToSASTranslation::combination(int* array, int n, int k, int i){
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

int HTNToSASTranslation::bin(int n, int k){
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

int HTNToSASTranslation::power(int n, int p){
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
 

void HTNToSASTranslation::sasPlus(){
  sasPlusBits = new bool[htn->numVars];
  sasPlusOffset = new int[htn->numVars];
  bitsToSP = new int[htn->numStateBits];
  bitAlone = new bool[htn->numStateBits];

  firstIndexSP = new int[htn->numVars];
  lastIndexSP = new int[htn->numVars];
  
  for (int i = 0; i < htn->numStateBits; i++){
    bitsToSP[i] = 0;
    bitAlone[i] = false;
  }
  for (int i = 0; i < htn->numVars; i++){
    sasPlusBits[i] = false;
    sasPlusOffset[i] = 0;
    firstIndexSP[i] = 0;
    lastIndexSP[i] = 0;
  }

  bool change = false;
  
  for (int i = 0; i < htn->numVars; i++){
    if (i > 0){
      sasPlusOffset[i] = sasPlusOffset[i-1];
      if (sasPlusBits[i-1]){
        sasPlusOffset[i]++;
      }
    }
    firstIndexSP[i] = htn->firstIndex[i] + sasPlusOffset[i];
    lastIndexSP[i] = htn->lastIndex[i] + sasPlusOffset[i];
    if (htn->firstIndex[i] == htn->lastIndex[i]){
      change = true;
      sasPlusBits[i] = true;
      lastIndexSP[i]++;
    }
  }

  numStateBitsSP = lastIndexSP[htn->numVars - 1] + 1;
  
  if (not(change)){
    cerr << " no change";
    return;
  }
  factStrsSP = new string[numStateBitsSP];
  for (int i = 0; i < htn->numVars; i++){
    for (int j = 0; j < htn->lastIndex[i] - htn->firstIndex[i] + 1; j++){
  	bitsToSP[htn->firstIndex[i] + j] = htn->firstIndex[i] + j + sasPlusOffset[i];
      factStrsSP[htn->firstIndex[i] + j + sasPlusOffset[i]] = htn->factStrs[htn->firstIndex[i] + j];
    }
    if (sasPlusBits[i]){
      factStrsSP[htn->firstIndex[i] + 1 + sasPlusOffset[i]] = string("no_") + string(htn->factStrs[htn->firstIndex[i]]);
      bitAlone[htn->firstIndex[i]] = true;
    }
  }
  
  strictMutexesSP = new int*[htn->numStrictMutexes];
  mutexesSP = new int*[htn->numMutexes];
  
  for (int i = 0; i < htn->numStrictMutexes; i++){
    strictMutexesSP[i] = new int[htn->strictMutexesSize[i]];
    for (int j = 0; j < htn->strictMutexesSize[i]; j++){
      strictMutexesSP[i][j] = bitsToSP[htn->strictMutexes[i][j]];
    }
  }
  for (int i = 0; i < htn->numMutexes; i++){
    mutexesSP[i] = new int[htn->mutexesSize[i]];
    for (int j = 0; j < htn->mutexesSize[i]; j++){
      mutexesSP[i][j] = bitsToSP[htn->mutexes[i][j]];
    }
  }
  
  numPrecsSP = new int[htn->numActions];
  numAddsSP = new int[htn->numActions];

  precListsSP = new int*[htn->numActions];
  addListsSP = new int*[htn->numActions];
  
  for (int i = 0; i < htn->numActions; i++){
    numPrecsSP[i] = htn->numPrecs[i];
    precListsSP[i] = new int[numPrecsSP[i]];
    for (int j = 0; j < numPrecsSP[i]; j++){
      precListsSP[i][j] = bitsToSP[htn->precLists[i][j]];
    }
    numAddsSP[i] = htn->numAdds[i];
    for (int j = 0; j < htn->numDels[i]; j++){
      if (bitAlone[htn->delLists[i][j]]){
        numAddsSP[i]++;
      }
    }
    addListsSP[i] = new int[numAddsSP[i]];
    for (int j = 0; j < htn->numAdds[i]; j++){
      addListsSP[i][j] = bitsToSP[htn->addLists[i][j]];
    }
    int k = htn->numAdds[i];
    for (int j = 0; j < htn->numDels[i]; j++){
      if (bitAlone[htn->delLists[i][j]]){
        addListsSP[i][k] = bitsToSP[htn->delLists[i][j]] + 1;
        k++;
      }
    }
  }

  s0ListSP = new int[htn->numVars];
  for (int i = 0; i < htn->numVars; i++){
    s0ListSP[i] = lastIndexSP[i];
  }
  for (int i = 0; i < htn->s0Size; i++){
    int j = 0;
    while (true){
      if (j >= htn->s0Size || htn->firstIndex[j] > htn->s0List[i]){
        j--;
        break;
      }
      j++;
    }
    s0ListSP[j] = bitsToSP[htn->s0List[i]];
  }
  gListSP = new int[htn->gSize];
  for (int i = 0; i < htn->gSize; i++){
     gListSP[i] = bitsToSP[htn->gList[i]];
  }
  
  htn->s0Size = htn->numVars;
  htn->numStateBits = numStateBitsSP;
  delete[] htn->factStrs;
  htn->factStrs = factStrsSP;
  
  delete[] htn->firstIndex;
  htn->firstIndex = firstIndexSP;
  delete[] htn->lastIndex;
  htn->lastIndex = lastIndexSP;
  delete[] htn->strictMutexes;
  htn->strictMutexes = strictMutexesSP;
  delete[] htn->mutexes;
  htn->mutexes = mutexesSP;
  delete[] htn->precLists;
  htn->precLists = precListsSP;
  delete[] htn->addLists;
  htn->addLists = addListsSP;
  delete[] htn->numPrecs;
  htn->numPrecs = numPrecsSP;
  delete[] htn->numAdds;
  htn->numAdds = numAddsSP;
  delete[] htn->s0List;
  htn->s0List = s0ListSP;
  delete[] htn->gList;
  htn->gList = gListSP;

}

int HTNToSASTranslation::calculatePrecsAndAdds(int* s, int* p, int* a, string tasks, int** conv){
  int index = tasks.find(",");
  if (index > tasks.length()){
    int task = stoi(tasks.substr(1, tasks.length() - 2));
    s[0] = htn->numPrecs[task];
    s[1] = htn->numAdds[task];
    for (int i = 0; i < s[0]; i++){
      p[i] = htn->precLists[task][i];
    }
    for (int i = 0; i < s[1]; i++){
      a[i] = htn->addLists[task][i];
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
  int** varChange = new int*[htn->numVars];
  for (int i = 0; i < htn->numVars; i++){
    varChange[i] = new int[2 * nT];
    for (int j = 0; j < 2 * nT; j++){
      varChange[i][j] = -1;
    }
  }
  for (int i = 0; i < nT; i++){
    for (int j = 0; j < htn->numPrecs[task[i]]; j++){
      varChange[conv[htn->precLists[task[i]][j]][0]][2 * i] = htn->precLists[task[i]][j];
    }
    for (int j = 0; j < htn->numAdds[task[i]]; j++){
      varChange[conv[htn->addLists[task[i]][j]][0]][2 * i + 1] = htn->addLists[task[i]][j];
    }
  }
  s[0] = 0;
  s[1] = 0;
  for (int i = 0; i < htn->numVars; i++){
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
        for (int j = 0; j < htn->numVars; j++){
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
  for (int j = 0; j < htn->numVars; j++){
    delete[] varChange[j];
  }
  delete[] varChange;
  delete[] task;
  return 0;
}


void HTNToSASTranslation::reorderTasks(bool warning){
  if (warning) cout << "Warn" << endl;
  firstNumTOPrimTasks = new int[htn->numMethods];
  subTasksInOrder = new int*[htn->numMethods];
  hasNoLastTask = new bool[htn->numMethods];
  for (int i = 0; i < htn->numMethods; i++){
    firstNumTOPrimTasks[i] = 0;
    subTasksInOrder[i] = new int[htn->numSubTasks[i]];
    hasNoLastTask[i] = false;
    if (htn->numSubTasks[i] == 0){
      continue;
    }
    if (htn->numSubTasks[i] == 1){
      if (htn->isPrimitive[htn->subTasks[i][0]]){
        firstNumTOPrimTasks[i] = 1;
      }
      subTasksInOrder[i][0] = htn->subTasks[i][0];
      continue;
    }
    if (htn->numOrderings[i] == 0){
  	if (i != htn->taskToMethods[htn->initialTask][0] && warning){
  		cout << "ATTENTION: Instance is not of the parallel sequences type. Search with the PSeq encoding might be incomplete." << endl;
  	}
      for (int j = 0; j < htn->numSubTasks[i]; j++){
        subTasksInOrder[i][j] = htn->subTasks[i][htn->numSubTasks[i] - 1 - j];
      }
      hasNoLastTask[i] = true;
      continue;
    }
    int* subs = new int[htn->numSubTasks[i]];
    for (int j = 0; j < htn->numSubTasks[i]; j++){
  	bool valueSet = false;
      for (int k = 0; k < htn->numSubTasks[i]; k++){
        bool notAlone = false;
        for (int l = 0; l < htn->numOrderings[i] / 2; l++){
          bool alreadyUsed = false;
          bool ka = false;
          for (int m = 0; m < j; m++){
            if (k == subs[m]){
              ka = true;
              break;
            }
            if (htn->ordering[i][2 * l] == subs[m]){
              alreadyUsed = true;
              break;
            }
          }
          if (ka){
            notAlone = true;
            break;
          }
          if (not alreadyUsed && htn->ordering[i][2 * l + 1] == k){
            notAlone = true;
            break;
          }
        }
        if (not notAlone){
          if (not valueSet){
  			subs[j] = k;
  			valueSet = true;
  		} else if (warning){
  			cout << "ATTENTION: Instance is not of the parallel sequences type. Search with the PSeq encoding might be incomplete." << endl;
  		}
        }
      }
    }

    // reordering tasks and adjusting restrictions
    for (int j = 0; j < htn->numSubTasks[i]; j++){
      subTasksInOrder[i][j] = htn->subTasks[i][htn->numSubTasks[i] - 1 - subs[j]];
    }
    for (int j = 0; j < htn->numOrderings[i]; j++){
      htn->ordering[i][j] = htn->numSubTasks[i] - 1 - subs[htn->ordering[i][j]];
    }
    // evaluating number of primitive tasks in front
    for (int j = 0; j < htn->numSubTasks[i]; j++){
      if (htn->isPrimitive[htn->subTasks[i][j]]){
        firstNumTOPrimTasks[i]++;
      }
      else {
        break;
      }
    }
    
    // checking if every method has a last task
    if (htn->numOrderings[i] / 2 < htn->numSubTasks[i] - 1){
      hasNoLastTask[i] = true;
      cerr << i << " has not enough constraints" << endl;
    }
    else {
      bool* lower = new bool[htn->numSubTasks[i]];
      for (int j = 0; j < htn->numSubTasks[i] - 1; j++){
        lower[j] = false;
      }
      lower[htn->numSubTasks[i] - 1] = true;
      for (int j = 0; j < htn->numSubTasks[i] - 1; j++){
        for (int k = 0; k < htn->numOrderings[i] / 2; k++){
          if (lower[htn->ordering[i][2 * k]]){
            lower[htn->ordering[i][2 * k + 1]] = true;
          }
        }
      }
      for (int j = 0; j < htn->numSubTasks[i]; j++){
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
//////


int HTNToSASTranslation::minProgressionBound(){
  int i = minImpliedPGB[htn->initialTask];
  if (i < 1){
    return 1;
  }
  return i;
}
int HTNToSASTranslation::maxProgressionBound(){
  return 5000000;
}


struct tOrMnode {
	bool isMethod = false;
	int id;
};



void HTNToSASTranslation::calcMinimalProgressionBound(bool to) {

    timeval tp;
    gettimeofday(&tp, NULL);
    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    cout << "- Calculating minimal Progression Bound";

    this->minImpliedPGB = new int[htn->numTasks];
    int* minImpliedPGBM = new int[htn->numMethods];

    for(int i = 0; i < htn->numMethods; i++) {
        minImpliedPGBM[i] = 0;
    }

    for(int i = 0; i < htn->numTasks; i++){
        if(i < htn->numActions){
            minImpliedPGB[i] = 1;
        } else {
            minImpliedPGB[i] = 0;
        }
    }

    list<tOrMnode*> h;
    for(int i = 0; i < htn->numActions; i++) {
        for(int j = 0; j < htn->stToMethodNum[i]; j++) {
            int m = htn->stToMethod[i][j];
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

            minImpliedPGBM[n->id] = htn->numSubTasks[n->id] - off; // number of tasks added to the stack
            if (minImpliedPGBM[n->id] < 1){
              minImpliedPGBM[n->id] = 1;
            }
            int minAdditionalDistance = 0;
            for (int i = 0; i < htn->numSubTasks[n->id] - off; i++) {
              int st = this->subTasksInOrder[n->id][i];
              if (st == htn->decomposedTask[n->id]){
                int k = htn->numSubTasks[n->id] - 1 - i - off;
                if (minAdditionalDistance < k){
                  minAdditionalDistance = htn->numSubTasks[n->id] - 1 - i - off;
                }
              }
              int add = minImpliedPGB[st] - htn->numSubTasks[n->id] + i + off;
              if (add > minAdditionalDistance){
                minAdditionalDistance = add;
              }
            }
            minImpliedPGBM[n->id] += minAdditionalDistance;
            bool changed = ((minImpliedPGBM[n->id] != cDist));
            if (changed) {
                tOrMnode* nn = new tOrMnode();
                nn->id = htn->decomposedTask[n->id];
                nn->isMethod = false;
                h.push_back(nn);
            }
        } else { // is task
            int cDist = minImpliedPGB[n->id];

            minImpliedPGB[n->id] = INT_MAX;
            for (int i = 0; i < htn->numMethodsForTask[n->id]; i++) {
                int m = htn->taskToMethods[n->id][i];
                minImpliedPGB[n->id] = min(minImpliedPGBM[m], minImpliedPGB[n->id]);
            }

            bool changed = ((minImpliedPGB[n->id] != cDist));
            if (changed) {
                for(int i = 0; i < htn->stToMethodNum[n->id]; i++){
                    int m = htn->stToMethod[n->id][i];
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


/////////////////////////////////////////////// actual encodings
int HTNToSASTranslation::htnToCondSorted(int pgb) {
  // number of translated variables
  int n = pgb * (pgb - 1) / 2;
  numVarsTrans = htn->numVars + pgb * 3 + n;

  // indizes for variables
  firstVarIndex = 0;
  firstTaskIndex = htn->numVars;
  firstConstraintIndex = htn->numVars + pgb;
  firstStackIndex = firstConstraintIndex + n;
  int firstMoveIndex = firstStackIndex + pgb;
  numInvalidTransActions = 0;

  // indizes for names
  firstIndexTrans = new int[numVarsTrans];
  lastIndexTrans = new int[numVarsTrans];

  for (int i = 0; i < htn->numVars; i++){
    firstIndexTrans[firstVarIndex+ i] = htn->firstIndex[i];
    lastIndexTrans[firstVarIndex + i] = htn->lastIndex[i];
  }
  
  for (int i = 0; i < pgb; i++){
    firstIndexTrans[firstTaskIndex + i] = lastIndexTrans[firstTaskIndex + i - 1] + 1;
    lastIndexTrans[firstTaskIndex + i] = firstIndexTrans[firstTaskIndex + i] + htn->numTasks;
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
  
  for (int i = 0; i < htn->numStateBits; i++){
    factStrsTrans[firstIndexTrans[firstVarIndex] + i] = htn->factStrs[i];
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
  s0ListTrans[0] = firstIndexTrans[firstTaskIndex] + htn->initialTask + 1;
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
    int m = bin(pgb - 1, htn->numSubTasks[i - 1] - 1);
    if (m < 1){
      m = 1;
    }
    if (m == INT_MAX){
      return -1;
    }
    methodIndexes[i] = methodIndexes[i - 1] + m;
  }
  */

  numActionsTrans = (htn->numActions + htn->numMethods) * pgb + htn->numTasks * (pgb - 1);
  firstMethodIndex = htn->numActions * pgb;
  int methodTaskIndex = (htn->numActions + htn->numMethods) * pgb;
  actionCostsTrans = new int[numActionsTrans];
  invalidTransActions = new bool[numActionsTrans];
  for (int i = 0; i < numActionsTrans; i++) {
    invalidTransActions[i] = false;
  }
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      actionCostsTrans[i * pgb + j] = htn->actionCosts[i];
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
  for (int i = 0; i < htn->numMethods; i++) {
    for (int j = 0; j < pgb; j++){
      int index = firstMethodIndex + i * pgb + j;
      if (htn->numSubTasks[i] > 1){
        numConditionalEffectsTrans[index] = (htn->numSubTasks[i] - 1) * j + htn->numOrderings[i] / 2;
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
  for (int i = 0; i < htn->numTasks; i++) {
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
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      int index = i * pgb + j;
      numPrecsTrans[index] = htn->numPrecs[i] + pgb;
      numAddsTrans[index] = htn->numAdds[i] + 2 + j;
      precListsTrans[index] = new int[numPrecsTrans[index]];
      addListsTrans[index] = new int[numAddsTrans[index]];
      for (int k = 0; k < htn->numPrecs[i]; k++){
        precListsTrans[index][k] = htn->precLists[i][k];
      }
      for (int k = 0; k < htn->numAdds[i]; k++){
        addListsTrans[index][k] = htn->addLists[i][k];
      }
      for (int k = 0; k < pgb; k++){
        if (k < j){
          precListsTrans[index][htn->numPrecs[i] + k] = firstIndexTrans[firstStackIndex + k] + 1;
          addListsTrans[index][htn->numAdds[i] + k] = firstIndexTrans[firstConstraintIndex + j * (j - 1) / 2 + k];
        }
        else if (k > j) {
          precListsTrans[index][htn->numPrecs[i] + k] = firstIndexTrans[firstConstraintIndex + k * (k - 1) / 2 + j];
        }
        else {
          precListsTrans[index][htn->numPrecs[i] + k] = firstIndexTrans[firstTaskIndex + j] + i + 1;
        }
      }
      addListsTrans[index][htn->numAdds[i] + j] = firstIndexTrans[firstTaskIndex + j];
      addListsTrans[index][htn->numAdds[i] + j + 1] = firstIndexTrans[firstStackIndex + j];
    }
  }    

 
  taskToKill = new int[htn->numMethods];
  for (int i = 0; i < htn->numTasks; i++){
    for (int j = 0; j < htn->numMethodsForTask[i]; j++){
      taskToKill[htn->taskToMethods[i][j]] = i + 1;
    }
  }
  
  // transformed methods
  for (int i = 0; i < htn->numMethods; i++) {
    for (int j = 0; j < pgb; j++){
      int index = firstMethodIndex + i * pgb + j;
      if (htn->numSubTasks[i] == 0){
        numAddsTrans[index] = 2 + j;
        numPrecsTrans[index] = pgb;
      }
      else if (htn->numSubTasks[i] == 1){
        numAddsTrans[index] = 1;
        numPrecsTrans[index] = pgb;
      }
      else {
        numAddsTrans[index] = htn->numSubTasks[i] * 2 - 1;
        numPrecsTrans[index] = htn->numSubTasks[i] - 1 + pgb;
      }
      precListsTrans[index] = new int[numPrecsTrans[index]];
      addListsTrans[index] = new int[numAddsTrans[index]];

      if (htn->numSubTasks[i] + j > pgb){
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
      
      if (htn->numSubTasks[i] == 0){
        for (int k = 0; k < j; k++){
          addListsTrans[index][k] = firstIndexTrans[firstConstraintIndex + j * (j - 1) / 2 + k];
        }
        addListsTrans[index][j] = firstIndexTrans[firstTaskIndex + j];
        addListsTrans[index][j + 1] = firstIndexTrans[firstStackIndex + j];
      }
      else if (htn->numSubTasks[i] == 1){
        addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
      }
      else {
        addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
        for (int l = 0; l < htn->numSubTasks[i] - 1; l++){
          precListsTrans[index][l + pgb] = firstIndexTrans[firstTaskIndex + pgb - 1 - l];
          addListsTrans[index][l + 1] = firstIndexTrans[firstTaskIndex + pgb - htn->numSubTasks[i] + 1 + l] + 1 + subTasksInOrder[i][l + 1];
          addListsTrans[index][l + htn->numSubTasks[i]] = firstIndexTrans[firstStackIndex + pgb - 1 - l] + 1;
          for (int m = 0; m < j; m++){
            effectConditionsTrans[index][l * j + m][0] = firstIndexTrans[firstConstraintIndex + j * (j - 1) / 2 + m] + 1;
            effectsTrans[index][l * j + m] = firstIndexTrans[firstConstraintIndex + (pgb - l - 1) * (pgb - l - 2) / 2 + m] + 1;
          }
        }
        for (int l = 0; l < htn->numOrderings[i] / 2; l++){
          int first = htn->ordering[i][2 * l] - 1;
          int second = htn->ordering[i][2 * l + 1] - 1;
          if (first < 0){
            first = j;
          }
          else {
            first = pgb - htn->numSubTasks[i] + first + 1;
          }
          if (second < 0){
            second = j;
          }
          else {
            second = pgb - htn->numSubTasks[i] + second + 1;
          }
          effectConditionsTrans[index][j * (htn->numSubTasks[i] - 1) + l][0] = firstIndexTrans[firstConstraintIndex + first * (first - 1) / 2 + second];
          effectsTrans[index][j * (htn->numSubTasks[i] - 1) + l] = firstIndexTrans[firstConstraintIndex + first * (first - 1) / 2 + second] + 1;
        }
      }
    }
  }

  // sort queue
  index = methodTaskIndex;
  for (int i = 0; i < htn->numTasks; i++) {
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
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      actionNamesTrans[i * pgb + j] = "primitive(id[" + to_string(i) + "],head[" + to_string(j) + "]): " + htn->taskNames[i];
    }
  }
  for (int i = 0; i < htn->numMethods; i++) {
    for (int j = 0; j < pgb; j++){
      int index = firstMethodIndex + i * pgb + j;
      if (invalidTransActions[index]){
        continue;
      }
      actionNamesTrans[index] = "method(id[" + to_string(i) + "],head[" + to_string(j);
      if (htn->numSubTasks[i] > 0){
        actionNamesTrans[index] += "],subtasks[" + to_string(j);
        for (int l = 0; l < htn->numSubTasks[i] - 1; l++){
          if (l < htn->numSubTasks[i] - 1){
            actionNamesTrans[index] += ",";
          }
          actionNamesTrans[index] += to_string(pgb - htn->numSubTasks[i] + 1 + l);
        }
        actionNamesTrans[index] += "]): ";
        actionNamesTrans[index] += htn->methodNames[i];
      }
    }
  }
  
  index = methodTaskIndex;
  for (int i = 0; i < htn->numTasks; i++) {
    for (int j = 1; j < pgb; j++){
      actionNamesTrans[index] = "move(task[" + to_string(i) + "],from[" + to_string(j) + "],to[" + to_string(j - 1) + "]";
      index++;
    }
  }
  return numActionsTrans;
}


int HTNToSASTranslation::tohtnToStrips(int pgb) {
  // number of translated variables
  numVarsTrans = htn->numVars + 1 + pgb;
  // indizes for variables
  firstVarIndex = 0;
  headIndex = htn->numVars;
  firstTaskIndex = htn->numVars + 1;
  varNamesTrans = new string[numVarsTrans];
  numInvalidTransActions = 0;


  // indizes for names
  firstIndexTrans = new int[numVarsTrans];
  lastIndexTrans = new int[numVarsTrans];

  for (int i = 0; i < htn->numVars; i++){
    firstIndexTrans[firstVarIndex+ i] = htn->firstIndex[i];
    lastIndexTrans[firstVarIndex+i] = htn->lastIndex[i];
  }

  firstIndexTrans[headIndex] = lastIndexTrans[headIndex - 1] + 1;
  lastIndexTrans[headIndex] = firstIndexTrans[headIndex] + pgb;

  for (int i = 0; i < pgb; i++){
    firstIndexTrans[firstTaskIndex + i] = lastIndexTrans[firstTaskIndex + i - 1] + 1;
    lastIndexTrans[firstTaskIndex + i] = firstIndexTrans[firstTaskIndex + i] + htn->numTasks;
	//cout << firstIndexTrans[firstTaskIndex + i] << " " << lastIndexTrans[firstTaskIndex + i] << endl;
  }

  numStateBitsTrans = lastIndexTrans[numVarsTrans - 1] + 1;

  factStrsTrans = new string[numStateBitsTrans];


  varNamesTrans[headIndex] = "varHead";
  for (int i = firstTaskIndex; i < numVarsTrans ; i++){
    varNamesTrans[i] = "varTasksHead" + to_string(i);
  }
  for (int i = 0; i < htn->numVars; i++){
    varNamesTrans[firstVarIndex + i] = htn->varNames[i];
  }


  for (int i = 0; i < htn->numStateBits; i++){
    factStrsTrans[firstIndexTrans[firstVarIndex] + i] = htn->factStrs[i];
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
  s0ListTrans[1] = firstIndexTrans[firstTaskIndex] + htn->initialTask + 1;
  for (int i = 1; i < s0SizeTrans - 1; i++){
    s0ListTrans[i + 1] = firstIndexTrans[firstTaskIndex + i];
  }

  // Goal state
  gSizeTrans = 1;
  gListTrans = new int[gSizeTrans];
  gListTrans[0] = firstIndexTrans[headIndex];

  // transformed actions and methods
  numActionsTrans = htn->numActions * pgb + htn->numMethods * pgb;
  firstMethodIndex = htn->numActions * pgb;
  actionCostsTrans = new int[numActionsTrans];
  invalidTransActions = new bool[numActionsTrans];
  for (int i = 0; i < numActionsTrans; i++) {
    invalidTransActions[i] = false;
  }
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      actionCostsTrans[i * pgb + j] = htn->actionCosts[i];
    }
  }
  for (int i = firstMethodIndex; i < numActionsTrans; i++) {
    actionCostsTrans[i] = 0;
    int method = (i-firstMethodIndex)/pgb;
    //cout << "ACC " << i << " " << method << " " << firstNumTOPrimTasks[method] << " " << numSubTasks[method] << " " << firstNumTOPrimTasks[method] << endl;
    if (firstNumTOPrimTasks[method] > 0){
        for (int k = 0; k < firstNumTOPrimTasks[method]; k++){
  		  //cout << "C " << method << " " << numMethods << " " << numSubTasks[method] << " " << 1 << " " << k << " = " << subTasksInOrder[method][numSubTasks[method] - 1 - k] << endl;
      	  actionCostsTrans[i] += htn->actionCosts[subTasksInOrder[method][htn->numSubTasks[method] - 1 - k]];
  	  }
    }
  }

  // transformed actions
  numPrecsTrans = new int[numActionsTrans];
  numAddsTrans = new int[numActionsTrans];
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      numPrecsTrans[i * pgb + j] = htn->numPrecs[i] + 2;
      numAddsTrans[i * pgb + j] = htn->numAdds[i] + 2;
    }
  }

  for (int i = 0; i < htn->numMethods; i++) {
    for (int j = 0; j < pgb; j++){
      if (htn->numSubTasks[i] + j - firstNumTOPrimTasks[i] > pgb){
        invalidTransActions[firstMethodIndex + i * pgb + j] = true;
      }
      numPrecsTrans[firstMethodIndex + i * pgb + j] = 2;
      if (htn->numSubTasks[i] - firstNumTOPrimTasks[i] == 0){
        numAddsTrans[firstMethodIndex + i * pgb + j] = 2;
      }
      else if (htn->numSubTasks[i] - firstNumTOPrimTasks[i] == 1){
        numAddsTrans[firstMethodIndex + i * pgb + j] = 1;
      }
      else {
        numAddsTrans[firstMethodIndex + i * pgb + j] = htn->numSubTasks[i] - firstNumTOPrimTasks[i] + 1;
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
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      for (int k = 0; k < htn->numPrecs[i]; k++){
        precListsTrans[i * pgb + j][k] = htn->precLists[i][k];
      }
      precListsTrans[i * pgb + j][numPrecsTrans[i * pgb + j] - 1] = firstIndexTrans[headIndex] + 1 + j;
      precListsTrans[i * pgb + j][numPrecsTrans[i * pgb + j] - 2] = firstIndexTrans[firstTaskIndex + j] + 1 + i;
      for (int k = 0; k < htn->numAdds[i]; k++){
        addListsTrans[i * pgb + j][k] = htn->addLists[i][k];
      }
      addListsTrans[i * pgb + j][numAddsTrans[i * pgb + j] - 1] = firstIndexTrans[headIndex] + j;
      addListsTrans[i * pgb + j][numAddsTrans[i * pgb + j] - 2] = firstIndexTrans[firstTaskIndex + j];
    }
  }
  taskToKill = new int[htn->numMethods];
  for (int i = 0; i < htn->numTasks; i++){
    for (int j =0; j < htn->numMethodsForTask[i]; j++){
      taskToKill[htn->taskToMethods[i][j]] = i + 1;
    }
  }
  //precs, dels, adds for methods
  for (int i = 0; i < htn->numMethods; i++) {
      for (int j = 0; j < pgb; j++){
          if (invalidTransActions[firstMethodIndex + i * pgb + j]){
              continue;
          }
          precListsTrans[firstMethodIndex + i * pgb + j][0] = firstIndexTrans[headIndex] + 1 + j;
          precListsTrans[firstMethodIndex + i * pgb + j][1] = firstIndexTrans[firstTaskIndex + j] + taskToKill[i];
          if (htn->numSubTasks[i] - firstNumTOPrimTasks[i] == 0){
            addListsTrans[firstMethodIndex + i * pgb + j][1] = firstIndexTrans[firstTaskIndex + j];
            addListsTrans[firstMethodIndex + i * pgb + j][0] = firstIndexTrans[headIndex] + j;
            continue;
          }
          if (htn->numSubTasks[i] - firstNumTOPrimTasks[i] == 1){
            addListsTrans[firstMethodIndex + i * pgb + j][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
            continue;
          }
          for (int k = 0; k < htn->numSubTasks[i] - firstNumTOPrimTasks[i]; k++){
            addListsTrans[firstMethodIndex + i * pgb + j][k] = firstIndexTrans[firstTaskIndex + j + k] + 1 + subTasksInOrder[i][k];
          }
          addListsTrans[firstMethodIndex + i * pgb + j][numAddsTrans[firstMethodIndex + i * pgb + j] - 1] = firstIndexTrans[headIndex] + j + htn->numSubTasks[i] - firstNumTOPrimTasks[i];
      }
  }
  // names
  actionNamesTrans = new string[numActionsTrans];
  for (int i = 0; i < htn->numActions; i++) {
      for (int j = 0; j < pgb; j++){
          actionNamesTrans[i * pgb + j] = "primitive(id[" + to_string(i)  + "],head[" + to_string(j) + "]): " + htn->taskNames[i];
      }
  }
  for (int i = 0; i < htn->numMethods; i++) {
      for (int j = 0; j < pgb; j++){
          actionNamesTrans[firstMethodIndex + i * pgb + j] = "method(id[" + to_string(i) + "],head[" + to_string(j);
          if (htn->numSubTasks[i] - firstNumTOPrimTasks[i] > 0){
            actionNamesTrans[firstMethodIndex + i * pgb + j] += "],subtasks[" + to_string(j);
            for (int k = 1; k < htn->numSubTasks[i] - firstNumTOPrimTasks[i]; k++){
              actionNamesTrans[firstMethodIndex + i * pgb + j] += ',' + to_string(j + k);
            }
          }
          
  		if (firstNumTOPrimTasks[i] > 0){
            actionNamesTrans[firstMethodIndex + i * pgb + j] += "],firstPrim[" + to_string(subTasksInOrder[i][htn->numSubTasks[i] - 1]);
            for (int k = 1; k < firstNumTOPrimTasks[i]; k++){
              actionNamesTrans[firstMethodIndex + i * pgb + j] += ',' + to_string(subTasksInOrder[i][htn->numSubTasks[i] - 1 - k]);
            }
          }
          actionNamesTrans[firstMethodIndex + i * pgb + j] += "]): " + htn->methodNames[i];
  		//cout << "output " << i << " " << j << " " << firstMethodIndex + i * pgb + j << " " << " " << firstNumTOPrimTasks[i] << actionNamesTrans[firstMethodIndex + i * pgb + j] << endl;
      }
  }
  return numActionsTrans;
}


int HTNToSASTranslation::htnPS(int numSeq, int* pgbList) {
  // number of variables: one head counter per sequence, the sequences themselves and constraints between sequences
  int pgb = 0;
  numVarsTrans = htn->numVars + numSeq + 1 + numSeq * (numSeq - 1);
  for (int i = 0; i < numSeq; i++){
    numVarsTrans += pgbList[i];
    if (pgb < pgbList[i]){
      pgb = pgbList[i];
    }
  }
  // relevant indezes
  firstVarIndex = 0;
  headIndex = htn->numVars + 1;
  firstConstraintIndex = htn->numVars + numSeq + 1;
  firstTaskIndex = htn->numVars + numSeq + 1 + numSeq * (numSeq - 1);
  varNamesTrans = new string[numVarsTrans];

  firstIndexTrans = new int[numVarsTrans];
  lastIndexTrans = new int[numVarsTrans];
  int* taskIndezes = new int[numSeq];
  
  
  for (int i = 0; i < htn->numVars; i++){
    firstIndexTrans[firstVarIndex + i] = htn->firstIndex[i];
    lastIndexTrans[firstVarIndex + i] = htn->lastIndex[i];
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
      lastIndexTrans[index] = firstIndexTrans[index] + htn->numTasks;
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
  for (int i = 0; i < htn->numVars; i++){
    varNamesTrans[firstVarIndex + i] = htn->varNames[i];
  }

  // state bit names
  for (int i = 0; i < htn->numStateBits; i++){
    factStrsTrans[firstIndexTrans[firstVarIndex] + i] = htn->factStrs[i];
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
      for (int k = 0; k < htn->numTasks; k++){
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
  numActionsTrans = htn->numActions * pgb * numSeq + htn->numMethods * pgb * numSeq;
  firstMethodIndex = htn->numActions * pgb * numSeq;

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
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      for (int k = 0; k < numSeq; k++){
        actionCostsTrans[index] = htn->actionCosts[i];
        if (j >= pgbList[k]){
          invalidTransActions[index] = true;
          numInvalidTransActions++;
          index++;
          continue;
        }
        numPrecsTrans[index] = htn->numPrecs[i] + 2 + numSeq - 1;
        numAddsTrans[index] = htn->numAdds[i] + 2;
        if (j == 0){
          numAddsTrans[index] += numSeq - 1;
        }
        precListsTrans[index] = new int[numPrecsTrans[index]];
        addListsTrans[index] = new int[numAddsTrans[index]];
        for (int l = 0; l < htn->numPrecs[i]; l++){
          precListsTrans[index][l] = htn->precLists[i][l];
        }
        for (int l = 0; l < htn->numAdds[i]; l++){
          addListsTrans[index][l] = htn->addLists[i][l];
        }
        for (int l = 0; l < numSeq - 1; l++){
          int off = 0;
          if (l >= k){
            off = 1;
          }
          precListsTrans[index][htn->numPrecs[i] + 2 + l] = firstIndexTrans[firstConstraintIndex + (l + off) * (numSeq - 1) + k - 1 + off];
          if (j == 0){
            addListsTrans[index][htn->numAdds[i] + 2 + l] = firstIndexTrans[firstConstraintIndex + (numSeq - 1) * k + l];
          }
        }
        precListsTrans[index][htn->numPrecs[i]] = firstIndexTrans[headIndex + k] + 1 + j;
        precListsTrans[index][htn->numPrecs[i] + 1] = firstIndexTrans[taskIndezes[k] + j] + 1 + i;
        addListsTrans[index][htn->numAdds[i]] = firstIndexTrans[headIndex + k] + j;
        addListsTrans[index][htn->numAdds[i] + 1] = firstIndexTrans[taskIndezes[k] + j];

        actionNamesTrans[index] = "primitive(id[" + to_string(i)  + "],head[" + to_string(k * pgb + j) + "]): " + htn->taskNames[i];
        index++;
      }
    }
  }
  
  taskToKill = new int[htn->numMethods];
  for (int i = 0; i < htn->numTasks; i++){
    for (int j = 0; j < htn->numMethodsForTask[i]; j++){
      taskToKill[htn->taskToMethods[i][j]] = i + 1;
    }
  }

  // methods
  index = firstMethodIndex;
  for (int i = 0; i < htn->numMethods; i++) {
    for (int j = 0; j < pgb; j++){
      for (int k = 0; k < numSeq; k++){
        actionCostsTrans[index] = 0;
        if (htn->numSubTasks[i] + j > pgbList[k] && !(taskToKill[i] == htn->initialTask + 1)){
          invalidTransActions[index] = true;
          numInvalidTransActions++;
          index++;
          continue;
        }
        numPrecsTrans[index] = 2 + numSeq - 1;
        if (htn->numSubTasks[i] == 0){
          numAddsTrans[index] = 2;
          if (j == 0){
            numAddsTrans[index] += numSeq - 1;
          }
        }
        else if (htn->numSubTasks[i] == 1){
          numAddsTrans[index] = 1;
        }
        else {
          numAddsTrans[index] = htn->numSubTasks[i] + 1;
        }
        // one top method
        if (taskToKill[i] == htn->initialTask + 1){
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
            for (int l = 0; l < htn->numOrderings[i] / 2; l++){
              int off = 0;
              if (htn->ordering[i][2 * l] < htn->ordering[i][2 * l + 1]){
                off = 1;
              }
              int order = htn->ordering[i][2 * l] * (numSeq - 1) + htn->ordering[i][2 * l + 1] - off;
              addListsTrans[index][1 + 2 * numSeq + order] = firstIndexTrans[firstConstraintIndex + order] + 1;
            }
          }
          else {
            invalidTransActions[index] = true;
            numInvalidTransActions++;
          }
          actionNamesTrans[index] = "method(id[" + to_string(i) + "],head[" + to_string(k * pgb + j);
          actionNamesTrans[index] += "],subtasks[" + to_string(0);
          for (int l = 1; l < htn->numSubTasks[i]; l++){
            actionNamesTrans[index] += ',' + to_string(l * pgb);
          }
          actionNamesTrans[index] += "]): " + htn->methodNames[i];
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
          if (j == 0 && htn->numSubTasks[i] == 0){
            addListsTrans[index][2 + l] = firstIndexTrans[firstConstraintIndex + (numSeq - 1) * k + l];
          }
        }
        if (htn->numSubTasks[i] == 0){
          addListsTrans[index][0] = firstIndexTrans[headIndex] + j;
          addListsTrans[index][1] = firstIndexTrans[taskIndezes[k] + j];
        }
        else if (htn->numSubTasks[i] == 1){
          addListsTrans[index][0] = firstIndexTrans[taskIndezes[k] + j] + 1 + subTasksInOrder[i][0];
        }
        else {
          for (int l = 0; l < htn->numSubTasks[i]; l++){
            addListsTrans[index][l] = firstIndexTrans[taskIndezes[k] + j + l] + 1 + subTasksInOrder[i][l];
          }
          addListsTrans[index][numAddsTrans[index] - 1] = firstIndexTrans[headIndex + k] + j + htn->numSubTasks[i];
        }

        actionNamesTrans[index] = "method(id[" + to_string(i) + "],head[" + to_string(k * pgb + j);
        if (htn->numSubTasks[i] > 0){
          actionNamesTrans[index] += "],subtasks[" + to_string(k * pgb + j);
          for (int l = 1; l < htn->numSubTasks[i]; l++){
            actionNamesTrans[index] += ',' + to_string(k * pgb + j + l);
          }
        }
        actionNamesTrans[index] += "]): " + htn->methodNames[i];
        
        index++;
      }
    }
  }
  delete[] taskIndezes;
  return numActionsTrans;
}

int HTNToSASTranslation::htnToStrips(int pgb) {
  // number of translated variables
  int n = pgb * (pgb - 1);
  numVarsTrans = htn->numVars + 2 * pgb + n;

  // indizes for variables
  firstVarIndex = 0;
  firstTaskIndex = htn->numVars;
  firstConstraintIndex = htn->numVars + pgb;
  firstStackIndex = firstConstraintIndex + n;

  // indizes for names
  firstIndexTrans = new int[numVarsTrans];
  lastIndexTrans = new int[numVarsTrans];

  for (int i = 0; i < htn->numVars; i++){
    firstIndexTrans[firstVarIndex+ i] = htn->firstIndex[i];
    lastIndexTrans[firstVarIndex+i] = htn->lastIndex[i];
  }
  
  for (int i = 0; i < pgb; i++){
    firstIndexTrans[firstTaskIndex + i] = lastIndexTrans[firstTaskIndex + i - 1] + 1;
    lastIndexTrans[firstTaskIndex + i] = firstIndexTrans[firstTaskIndex + i] + htn->numTasks+1;
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
  for (int i = 0; i < htn->numStateBits; i++){
    factStrsTrans[firstIndexTrans[firstVarIndex] + i] = htn->factStrs[i];
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
  s0ListTrans[0] = firstIndexTrans[firstTaskIndex] + htn->initialTask + 2;
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
  methodIndexes = new int[htn->numMethods + 1];
  methodIndexes[0] = 0;
  for (int i = 1; i < htn->numMethods + 1; i++){     
    int off = 1;
    if (hasNoLastTask[i - 1]){
      off = 0;
    }
    int m = bin(pgb - 1, htn->numSubTasks[i - 1] - off);
    if (m < 1){
      m = 1;
    }
    if (m == INT_MAX){
      return -1;
    }
    if (htn->numSubTasks[i - 1] <= 1){
      m = 1;
    }
    methodIndexes[i] = methodIndexes[i - 1] + m;
  }
  
  numMethodsTrans = methodIndexes[htn->numMethods];
  numMethodsTrans *= pgb;
  numActionsTrans = htn->numActions * pgb + numMethodsTrans;
  firstMethodIndex = htn->numActions * pgb;
  actionCostsTrans = new int[numActionsTrans];
  invalidTransActions = new bool[numActionsTrans];
  for (int i = 0; i < numActionsTrans; i++) {
    invalidTransActions[i] = false;
  }
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      actionCostsTrans[i * pgb + j] = htn->actionCosts[i];
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

  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      int index = i * pgb + j;
      numPrecsTrans[index] = htn->numPrecs[i] + pgb;
      numAddsTrans[index] = htn->numAdds[i] + pgb + 1;
      precListsTrans[index] = new int[numPrecsTrans[index]];
      addListsTrans[index] = new int[numAddsTrans[index]];
      for (int l = 0; l < htn->numPrecs[i]; l++){
        precListsTrans[index][l] = htn->precLists[i][l];
      }
      for (int l = 0; l < htn->numAdds[i]; l++){
        addListsTrans[index][l] = htn->addLists[i][l];
      }
      for (int l = 0; l < pgb; l++){
        if (l < j){
          precListsTrans[index][htn->numPrecs[i] + l] = firstIndexTrans[firstConstraintIndex + l * (pgb - 1) + j - 1];
        }
        else if (j == l){
          precListsTrans[index][htn->numPrecs[i] + l] = firstIndexTrans[firstTaskIndex + j] + 2 + i;
        }
        else {
          precListsTrans[index][htn->numPrecs[i] + l] = firstIndexTrans[firstConstraintIndex + l * (pgb - 1) + j];
        }
      }
      actionNamesTrans[index] = "primitive(id[" + to_string(i) + "],head[" + to_string(j) + "]): " + htn->taskNames[i];
      addListsTrans[index][htn->numAdds[i]] = firstIndexTrans[firstTaskIndex + j];
      addListsTrans[index][numAddsTrans[index] - 1] = firstIndexTrans[firstStackIndex + j];
      for (int k = 0; k < pgb - 1; k++){
        addListsTrans[index][htn->numAdds[i] + 1 + k] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + k];
      }
    }
  }

  taskToKill = new int[htn->numMethods];
  for (int i = 0; i < htn->numTasks; i++){
    for (int j = 0; j < htn->numMethodsForTask[i]; j++){
      taskToKill[htn->taskToMethods[i][j]] = i + 2;
    }
  }
  // transformed methods
  for (int i = 0; i < htn->numMethods; i++) {
    int* subs = new int[htn->numSubTasks[i]];
    for (int j = 0; j < pgb; j++){
      for (int k = 0; k < (methodIndexes[i + 1] - methodIndexes[i]); k++){
        int index = firstMethodIndex + methodIndexes[i] * pgb + j * (methodIndexes[i + 1] - methodIndexes[i]) + k;
        int inv = 0;
        if (htn->numSubTasks[i] == 1){
          numAddsTrans[index] = 1;
          numPrecsTrans[index] = pgb;
        }
        else if (htn->numSubTasks[i] == 0){
          numAddsTrans[index] = pgb + 1;
          numPrecsTrans[index] = pgb;
        }
        else {
          if (hasNoLastTask[i]){
            inv = 1;
            combination(subs, pgb - 1, htn->numSubTasks[i], k);
            numAddsTrans[index] = htn->numSubTasks[i] * 3 + 1 + htn->numOrderings[i] / 2;
            numPrecsTrans[index] = htn->numSubTasks[i] + 1 + pgb + subs[htn->numSubTasks[i] - 1];
          }
          else {
            combination(subs, pgb - 1, htn->numSubTasks[i] - 1, k);
            numAddsTrans[index] = htn->numSubTasks[i] * 2 - 1 + htn->numOrderings[i] / 2;
            numPrecsTrans[index] = htn->numSubTasks[i] + pgb + subs[htn->numSubTasks[i] - 2];
          }
        }
        precListsTrans[index] = new int[numPrecsTrans[index]];
        addListsTrans[index] = new int[numAddsTrans[index]];
        if (htn->numSubTasks[i] + inv > pgb){
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
        if (htn->numSubTasks[i] == 0){
          addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j];
          addListsTrans[index][pgb] = firstIndexTrans[firstStackIndex + j];
          for (int m = 0; m < pgb - 1; m++){
            addListsTrans[index][1 + m] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + m];
          }
        }
        else if (htn->numSubTasks[i] == 1){
          addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 2 + subTasksInOrder[i][0];
        }
        else {
          if (hasNoLastTask[i]){
            addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1;
            for (int m = 0; m < subs[htn->numSubTasks[i] - 1] + 1; m++){
              int off = 0;
              if (m >= j){
                off = 1;
              }
              precListsTrans[index][htn->numSubTasks[i] + pgb + m] = firstIndexTrans[firstStackIndex + m + off] + 1;
            }
            for (int m = 0; m < htn->numSubTasks[i]; m++){
              int off = 0;
              if (subs[m] >= j){
                off = 1;
                subs[m]++;
              }
              precListsTrans[index][m + pgb] = firstIndexTrans[firstTaskIndex + subs[m]];
              precListsTrans[index][htn->numSubTasks[i] + pgb + subs[m] - off] = firstIndexTrans[firstStackIndex + subs[m]];
              addListsTrans[index][m + 1] = firstIndexTrans[firstTaskIndex + subs[m]] + 2 + subTasksInOrder[i][m];
              addListsTrans[index][m + htn->numSubTasks[i] + 1] = firstIndexTrans[firstConstraintIndex + (subs[m]) * (pgb - 1) + j - 1 + off] + 1;
              addListsTrans[index][m + htn->numSubTasks[i] * 2 + 1] = firstIndexTrans[firstStackIndex + (subs[m])] + 1;
            }
            for (int m = 0; m < htn->numOrderings[i] / 2; m++){
              int first = 0;
              int free = htn->ordering[i][2 * m];
              int constrained = htn->ordering[i][2 * m + 1];
              if (subs[constrained] >= subs[free]){
                first = 1;
              }
              addListsTrans[index][m + htn->numSubTasks[i] * 3 + 1] = lastIndexTrans[firstConstraintIndex + subs[free] * (pgb - 1) + subs[constrained] - first];
            }
          }
          else {
            addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + subTasksInOrder[i][0] + 2;
            for (int m = 0; m < subs[htn->numSubTasks[i] - 2] + 1; m++){
              int off = 0;
              if (m >= j){
                off = 1;
              }
              precListsTrans[index][htn->numSubTasks[i] + pgb - 1 + m] = firstIndexTrans[firstStackIndex + m + off] + 1;
            }
            for (int m = 0; m < htn->numSubTasks[i] - 1; m++){
              int off = 0;
              if (subs[m] >= j){
                off = 1;
                subs[m]++;
              }
              precListsTrans[index][m + pgb] = firstIndexTrans[firstTaskIndex + subs[m]];
              precListsTrans[index][htn->numSubTasks[i] + pgb - 1 + subs[m] - off] = firstIndexTrans[firstStackIndex + subs[m]];
              addListsTrans[index][m + 1] = firstIndexTrans[firstTaskIndex + subs[m]] + 2 + subTasksInOrder[i][m + 1];
              addListsTrans[index][m + htn->numSubTasks[i]] = firstIndexTrans[firstStackIndex + subs[m]] + 1;
            }
            for (int m = 0; m < htn->numOrderings[i] / 2; m++){
              int first = 0;
              int free = htn->ordering[i][2 * m] - 1;
              int constrained = htn->ordering[i][2 * m + 1] - 1;
              if (constrained == -1){
                if (j >= subs[free]){
                  first = 1;
                }
                addListsTrans[index][m + htn->numSubTasks[i] * 2 - 1] = lastIndexTrans[firstConstraintIndex + subs[free] * (pgb - 1) + j - first];
              }
              else {
                if (subs[constrained] >= subs[free]){
                  first = 1;
                }
                addListsTrans[index][m + htn->numSubTasks[i] * 2 - 1] = lastIndexTrans[firstConstraintIndex + subs[free] * (pgb - 1) + subs[constrained] - first];
              }
            }
          }
        }
        string name = "method(id[" + to_string(i) + "],head[" + to_string(j) + "],subtasks[";
        if (hasNoLastTask[i]){
          for (int l = 0; l < htn->numSubTasks[i]; l++){
            name += to_string(subs[l]);
            if (l < htn->numSubTasks[i] - 1){
              name += ",";
            }
          }
        }
        else {
          for (int l = 0; l < htn->numSubTasks[i]; l++){
            if (l == 0){
              name += to_string(j);
            }
            else {
              name += to_string(subs[l - 1]);
            }
            if (l < htn->numSubTasks[i] - 1){
              name += ",";
            }
          }
        }
        name += "]): " + htn->methodNames[i];
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


int HTNToSASTranslation::htnToCond(int pgb) {
  // number of translated variables
  int n = pgb * (pgb - 1);
  numVarsTrans = htn->numVars + pgb * 2 + n;

  // indizes for variables
  firstVarIndex = 0;
  firstTaskIndex = htn->numVars;
  firstConstraintIndex = htn->numVars + pgb;
  firstStackIndex = firstConstraintIndex + n;

  // indizes for names
  firstIndexTrans = new int[numVarsTrans];
  lastIndexTrans = new int[numVarsTrans];

  for (int i = 0; i < htn->numVars; i++){
    firstIndexTrans[firstVarIndex+ i] = htn->firstIndex[i];
    lastIndexTrans[firstVarIndex + i] = htn->lastIndex[i];
  }
  
  for (int i = 0; i < pgb; i++){
    firstIndexTrans[firstTaskIndex + i] = lastIndexTrans[firstTaskIndex + i - 1] + 1;
    lastIndexTrans[firstTaskIndex + i] = firstIndexTrans[firstTaskIndex + i] + htn->numTasks;
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
  
  for (int i = 0; i < htn->numStateBits; i++){
    factStrsTrans[firstIndexTrans[firstVarIndex] + i] = htn->factStrs[i];
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
  s0ListTrans[0] = firstIndexTrans[firstTaskIndex] + htn->initialTask + 1;
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
  methodIndexes = new int[htn->numMethods + 1];
  methodIndexes[0] = 0;
  for (int i = 1; i < htn->numMethods + 1; i++){
    int m = bin(pgb - 1, htn->numSubTasks[i - 1] - 1);
    if (m < 1){
      m = 1;
    }
    if (m == INT_MAX){
      return -1;
    }
    methodIndexes[i] = methodIndexes[i - 1] + m;
  }
  
  numMethodsTrans = methodIndexes[htn->numMethods];
  numMethodsTrans *= pgb;
  numActionsTrans = htn->numActions * pgb + numMethodsTrans;
  firstMethodIndex = htn->numActions * pgb;
  actionCostsTrans = new int[numActionsTrans];
  invalidTransActions = new bool[numActionsTrans];
  for (int i = 0; i < numActionsTrans; i++) {
    invalidTransActions[i] = false;
  }
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      actionCostsTrans[i * pgb + j] = htn->actionCosts[i];
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
  for (int i = 0; i < htn->numMethods; i++) {
    for (int j = 0; j < pgb; j++){
      for (int k = 0; k < (methodIndexes[i + 1] - methodIndexes[i]); k++){
        int index = firstMethodIndex + methodIndexes[i] * pgb + j * (methodIndexes[i + 1] - methodIndexes[i]) + k;
        if (htn->numSubTasks[i] > 1){
          numConditionalEffectsTrans[index] = (htn->numSubTasks[i] - 1) * (pgb - 1) + htn->numOrderings[i] / 2;
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
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      int index = i * pgb + j;
      numPrecsTrans[index] = htn->numPrecs[i] + pgb;
      numAddsTrans[index] = htn->numAdds[i] + pgb + 1;
      precListsTrans[index] = new int[numPrecsTrans[index]];
      addListsTrans[index] = new int[numAddsTrans[index]];
      
      for (int k = 0; k < htn->numPrecs[i]; k++){
        precListsTrans[index][k] = htn->precLists[i][k];
      }
      for (int k = 0; k < htn->numAdds[i]; k++){
        addListsTrans[index][k] = htn->addLists[i][k];
      }
      for (int k = 0; k < pgb; k++){
        if (k < j){
          precListsTrans[index][htn->numPrecs[i] + k] = firstIndexTrans[firstConstraintIndex + k * (pgb - 1) + j - 1];
        }
        else if (j == k){
          precListsTrans[index][htn->numPrecs[i] + k] = firstIndexTrans[firstTaskIndex + j] + 1 + i;
        }
        else {
          precListsTrans[index][htn->numPrecs[i] + k] = firstIndexTrans[firstConstraintIndex + k * (pgb - 1) + j];
        }
      }
      for (int k = 0; k < pgb - 1; k++){
        addListsTrans[index][htn->numAdds[i] + k] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + k];
      }
      addListsTrans[index][htn->numAdds[i] + pgb - 1] = firstIndexTrans[firstTaskIndex + j];
      addListsTrans[index][htn->numAdds[i] + pgb] = firstIndexTrans[firstStackIndex + j];
    }
  }    

 
  taskToKill = new int[htn->numMethods];
  for (int i = 0; i < htn->numTasks; i++){
      for (int j = 0; j < htn->numMethodsForTask[i]; j++){
          taskToKill[htn->taskToMethods[i][j]] = i + 1;
      }
  }
  
  // transformed methods
  for (int i = 0; i < htn->numMethods; i++) {
    int* subs = new int[htn->numSubTasks[i] -1];
    for (int j = 0; j < pgb; j++){
      for (int k = 0; k < (methodIndexes[i + 1] - methodIndexes[i]); k++){
        int index = firstMethodIndex + methodIndexes[i] * pgb + j * (methodIndexes[i + 1] - methodIndexes[i]) + k;
        if (htn->numSubTasks[i] == 0){
          numAddsTrans[index] = pgb + 1;
          numPrecsTrans[index] = pgb + 1;
        }
        else if (htn->numSubTasks[i] == 1){
          numAddsTrans[index] = 1;
          numPrecsTrans[index] = pgb;
        }
        else {
          combination(subs, pgb - 1, htn->numSubTasks[i] - 1, k);
          numAddsTrans[index] = htn->numSubTasks[i] * 2 - 1;
          numPrecsTrans[index] = htn->numSubTasks[i] + pgb + subs[htn->numSubTasks[i] - 2];
        }
        
        precListsTrans[index] = new int[numPrecsTrans[index]];
        addListsTrans[index] = new int[numAddsTrans[index]];
        
        if (htn->numSubTasks[i] > pgb){
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
        
        if (htn->numSubTasks[i] == 0){
          addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j];
          for (int l = 0; l < pgb - 1; l++){
            addListsTrans[index][l + 1] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + l];
          }
          addListsTrans[index][pgb] = firstIndexTrans[firstStackIndex + j];
        }
        else if (htn->numSubTasks[i] == 1){
          addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
        }
        else {
          addListsTrans[index][0] = firstIndexTrans[firstTaskIndex + j] + 1 + subTasksInOrder[i][0];
          for (int m = 0; m < subs[htn->numSubTasks[i] - 2] + 1; m++){
            int off = 0;
            if (m >= j){
              off = 1;
            }
            precListsTrans[index][htn->numSubTasks[i] + pgb - 1 + m] = firstIndexTrans[firstStackIndex + m + off] + 1;
          }
          for (int l = 0; l < htn->numSubTasks[i] - 1; l++){
            int off = 0;
            if (subs[l] >= j){
              off = 1;
              subs[l]++;
            }
            precListsTrans[index][l + pgb] = firstIndexTrans[firstTaskIndex + subs[l]];
            precListsTrans[index][htn->numSubTasks[i] + pgb - 1 + subs[l] - off] = firstIndexTrans[firstStackIndex + subs[l]];
            addListsTrans[index][l + 1] = firstIndexTrans[firstTaskIndex + subs[l]] + 1 + subTasksInOrder[i][l + 1];
            addListsTrans[index][l + htn->numSubTasks[i]] = firstIndexTrans[firstStackIndex + subs[l]] + 1;
            
            for (int m = 0; m < pgb - 1; m++){
              effectConditionsTrans[index][l * (pgb - 1) + m][0] = firstIndexTrans[firstConstraintIndex + j * (pgb - 1) + m] + 1;
              effectsTrans[index][l * (pgb - 1) + m] = firstIndexTrans[firstConstraintIndex + subs[l] * (pgb - 1) + m] + 1;
            }
          }
          for (int l = 0; l < htn->numOrderings[i] / 2; l++){
            int first = htn->ordering[i][2 * l] - 1;
            int second = htn->ordering[i][2 * l + 1] - 1;
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
            effectConditionsTrans[index][(pgb - 1) * (htn->numSubTasks[i] - 1) + l][0] = firstIndexTrans[firstConstraintIndex + first * (pgb - 1) + second];
            effectsTrans[index][(pgb - 1) * (htn->numSubTasks[i] - 1) + l] = firstIndexTrans[firstConstraintIndex + first * (pgb - 1) + second] + 1;
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
  for (int i = 0; i < htn->numActions; i++) {
    for (int j = 0; j < pgb; j++){
      actionNamesTrans[i * pgb + j] = "primitive(id[" + to_string(i) + "],head[" + to_string(j) + "]): " + htn->taskNames[i];
    }
  }
  for (int i = 0; i < htn->numMethods; i++) {
    int* subs = new int[htn->numSubTasks[i] - 1];
    for (int j = 0; j < pgb; j++){
      for (int k = 0; k < (methodIndexes[i + 1] - methodIndexes[i]); k++){
        int index = firstMethodIndex + methodIndexes[i] * pgb + j * (methodIndexes[i + 1] - methodIndexes[i]) + k;
        if (invalidTransActions[index]){
          continue;
        }
        actionNamesTrans[index] = "method(id[" + to_string(i) + "],head[" + to_string(j);
        if (htn->numSubTasks[i] > 0){
          combination(subs, pgb - 1, htn->numSubTasks[i] - 1, k);
          actionNamesTrans[index] += "],subtasks[" + to_string(j);
          for (int l = 0; l < htn->numSubTasks[i] - 1; l++){
            int off = subs[l];
            if (off >= j){
              off++;
            }
            if (l < htn->numSubTasks[i] - 1){
              actionNamesTrans[index] += ",";
            }
            actionNamesTrans[index] += to_string(off);
          }
          actionNamesTrans[index] += "]): ";
          actionNamesTrans[index] += htn->methodNames[i];
        }
      }
    }
  }
  return 0;
}





////////////////////////////// writer 
void HTNToSASTranslation::writeToFastDown(string sasName, bool hasCondEff, bool realCosts) {
    ofstream sasfile;
    sasfile.open (sasName);
    // version
    sasfile << "begin_version" << endl;
    sasfile << "3" << endl;
    sasfile << "end_version" << endl;
    sasfile <<  endl;
    // metric
    sasfile << "begin_metric" << endl;
	if (realCosts)
	    sasfile << "1" << endl;
	else
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
    sasfile <<  htn->numMutexes + htn->numStrictMutexes << endl;
    for (int i = 0; i < htn->numStrictMutexes; i++){
      int size = htn->strictMutexesSize[i];
      sasfile << "begin_mutex_group" << endl;
      sasfile << size << endl;
      for (int j = 0; j < size; j++){
        sasfile << convertMutexVars[htn->strictMutexes[i][j]][0] << " ";
        sasfile << convertMutexVars[htn->strictMutexes[i][j]][1] << endl;
      }
      sasfile << "end_mutex_group" << endl;
    }
    for (int i = 0; i < htn->numMutexes; i++){
      int size = htn->mutexesSize[i];
      sasfile << "begin_mutex_group" << endl;
      sasfile << size << endl;
      for (int j = 0; j < size; j++){
        sasfile << convertMutexVars[htn->mutexes[i][j]][0] << " ";
        sasfile << convertMutexVars[htn->mutexes[i][j]][1] << endl;
      }
      sasfile << "end_mutex_group" << endl;
    }
    sasfile <<  endl;
    // initial state
    sasfile << "begin_state" << endl;
    for (int i = 0; i < htn->s0Size; i++) {
      sasfile << convertMutexVars[htn->s0List[i]][1] << endl;
    }
    for (int i = 0; i < this->s0SizeTrans; i++) {
      sasfile << convertMutexVars[this->s0ListTrans[i]][1] << endl;
    }
    sasfile << "end_state" << endl;
    sasfile <<  endl;
    // goal state -> different for htn_to_strips
    sasfile << "begin_goal" << endl;
    sasfile << gSizeTrans + htn->gSize << endl;
    for (int i = 0; i < htn->gSize; i++){
      sasfile << convertMutexVars[htn->gList[i]][0] << " " << convertMutexVars[htn->gList[i]][1] << endl;
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
      int * primPList = new int[htn->numStateBits];
      int * primAList = new int[htn->numStateBits];
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
      if (hasCondEff){
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
















///////// Plan extraction

void HTNToSASTranslation::planToHddl(string infile, string outfile) {
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
    bool** subHeadsFixed = new bool*[methNumMax];
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
        
	    for (int j = methNum - 1; j >= 0; j--){
		  bool repl = false;
          for (int k = 0; k < htn->numSubTasks[methIndex[j]]; k++){
            if (subHeadsFixed[j][k] == false && subHeads[j][k] == heads[i]){
              subHeadsFixed[j][k] = true;
			  repl = true;
              break;
            }
          }
          if (repl){
            break;
          }
        }
	
		fout << primNum << " " << primitives[primNum] << endl;
        primNum++;
      }
      else if (plan[i].size() > 6 && string("method").compare(plan[i].substr(1, 6)) == 0){
        methods[methNum] = plan[i].substr(plan[i].find(": ") + 2, plan[i].length() - plan[i].find(": ") - 3);
        methIndex[methNum] = stoi(plan[i].substr(plan[i].find("id") + 3, plan[i].find("]", plan[i].find("id"), 1) - plan[i].find("id") - 3));
        order[i] = methNum + primNumMax;
        heads[i] = stoi(plan[i].substr(plan[i].find("head") + 5, plan[i].find("]", plan[i].find("head"), 1) - plan[i].find("head") - 5));
        subHeads[methNum] = new int[htn->numSubTasks[methIndex[methNum]]];
        subHeadsFixed[methNum] = new bool[htn->numSubTasks[methIndex[methNum]]];
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
            primitives[primNum] = htn->taskNames[firstPrimIndezes[methNum][j]];
            primIndex[primNum] = firstPrimIndezes[methNum][j];
            fout << primNum << " " << primitives[primNum] << endl;
            firstPrimIndezes[methNum][j] = primNum;
            primNum++;
          }
        }
        index = plan[i].find("subtasks");
        if (index < plan[i].length()){
          string s = plan[i].substr(index + 9, plan[i].find("]", index, 1) - index - 9);
          for (int j = 0; j < htn->numSubTasks[methIndex[methNum]]; j++){
            int f = s.find(",");
            if (f < s.length()){
              subHeads[methNum][j] = stoi(s.substr(0, f));
              s = s.substr(f + 1, s.length());
            }
            else {
              subHeads[methNum][j] = stoi(s);
            }
          	subHeadsFixed[methNum][j] = false;
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
          for (int k = 0; k < htn->numSubTasks[methIndex[j]]; k++){
            if (subHeadsFixed[j][k] == false && subHeads[j][k] == from){
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
      fout << order[o] << " " << htn->taskNames[htn->decomposedTask[m]] << " -> ";
      fout << methods[i] << " ";
      int fntopt = 0;
      if (hasPrimIndezes[i]){
        fntopt = firstNumTOPrimTasks[methIndex[i]];
        for (int j = 0; j < fntopt; j++){
          fout << firstPrimIndezes[i][j] << " ";
        }
      }
      for (int j = htn->numSubTasks[m] - 1 - fntopt; j >= 0; j--){
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
            if (subTasksInOrder[m][j] == htn->decomposedTask[methIndex[order[k]-primNumMax]]){
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




///// code for debugging only
void HTNToSASTranslation::checkFastDownwardPlan(string domain, string plan){

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
  
  htn->numVars = stoi(line);
  htn->s0List = new int[htn->numVars];
  while (true){
    getline(*inputStream, line);
    int index = line.find("begin_state");
    if (index < line.length()){
      break;
    }
  }
  cerr << "initializing variables" << endl;
  for (int i = 0; i < htn->numVars; i++){
    getline(*inputStream, line);
    htn->s0List[i] = stoi(line);
    
    cerr << "variable: " << i << " value: " << htn->s0List[i] << endl;
    
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
  htn->gSize = stoi(line);
  htn->gList = new int[htn->numVars];
  for (int i = 0; i < htn->numVars; i++){
    htn->gList[i] = -1;
  }
  for (int i = 0; i < htn->gSize; i++){
    getline(*inputStream, line);
    int index = line.find(' ');
    int a = stoi(line.substr(0, index));
    int b = stoi(line.substr(index + 1, line.length()));
    htn->gList[a] = b;
    
    cerr << "goal: " << a << " value: " << b << endl;
    
  }
  getline(*inputStream, line);
  getline(*inputStream, line);
  getline(*inputStream, line);
  
  htn->numActions = stoi(line);
  cerr << endl;
  cerr << "number of actions: " << htn->numActions << endl;
  htn->precLists = new int*[htn->numActions];
  htn->addLists = new int*[htn->numActions];
  
  htn->taskNames = new string[htn->numActions];
  for (int i = 0; i < htn->numActions; i++){
    htn->precLists[i] = new int[htn->numVars];
    htn->addLists[i] = new int[htn->numVars];
    for (int j = 0; j < htn->numVars; j++){
      htn->precLists[i][j] = -1;
      htn->addLists[i][j] = -1;
    }
    while (true){
      getline(*inputStream, line);
      int index = line.find("begin_operator");
      if (index < line.length()){
        break;
      }
    }
    getline(*inputStream, line);
    htn->taskNames[i] = line;
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
      htn->precLists[i][a] = b;
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
      htn->precLists[i][a] = b;
      htn->addLists[i][a] = c;
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
    for (int j = 0; j < htn->numActions; j++){
      if (htn->taskNames[j].compare(line.substr(1, line.length() - 2)) == 0){
        index = j;
        break;
      }
    }
    if (index == -1){
      cerr << "no method with this name found" << endl;
      return;
    }
    cerr << "State" << endl;
    for (int j = 0; j < htn->numVars; j++){
      bool correct = ((htn->precLists[index][j] == -1) || (htn->precLists[index][j] == htn->s0List[j]));
      if (!correct){
        cerr << "cant apply method because current state is: " << htn->s0List[j] << " but should be: " << htn->precLists[index][j] << endl;
        return;
      }
      if (htn->precLists[index][j] != -1){
        cerr << j << ": " << htn->s0List[j] << " == " << htn->precLists[index][j] << "   ";
      }
      if (htn->addLists[index][j] != -1){
        cerr << j << ": " << htn->s0List[j] << " -> " << htn->addLists[index][j];
        htn->s0List[j] = htn->addLists[index][j];
      }
      if (htn->precLists[index][j] != -1 || htn->addLists[index][j] != -1){
        cerr << endl;
      }
    }
    getline(*inputStream, line);
    i++;
  }
  cerr << "finished applying methods!" << endl;
  cerr << "checking goal" << endl;
  for (int i = 0; i < htn->numVars; i++){
    cerr << i << " " << htn->s0List[i] << " " << htn->gList[i] << endl;
  }
}


