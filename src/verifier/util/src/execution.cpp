#include "execution.h"

PlanExecution::PlanExecution(Model *htn, vector<int> plan) {
    this->executable = true;
    this->stateSeq.resize(plan.size() + 1);
    for (size_t i = 0; i < htn->s0Size; i++) {
        this->stateSeq[0].insert(htn->s0List[i]);
    }
    size_t i = 1;
    for (i = 1; i < this->stateSeq.size(); i++) {
        for (int e : this->stateSeq[i - 1]) {
            this->stateSeq[i].insert(e);
        }
        for (size_t j = 0; j < htn->numPrecs[plan[i - 1]]; j++) {
            int stateVar = htn->precLists[plan[i - 1]][j];
            if (!this->stateSeq[i].count(stateVar)) {
                this->executable = false;
                return;
            }
        }
        for (size_t j = 0; j < htn->numDels[plan[i - 1]]; j++) {
            int stateVar = htn->delLists[plan[i - 1]][j];
            if (this->stateSeq[i].count(stateVar)) {
                this->stateSeq[i].erase(stateVar);
            }
        }
        for (size_t j = 0; j < htn->numAdds[plan[i - 1]]; j++) {
            int stateVar = htn->addLists[plan[i - 1]][j];
            this->stateSeq[i].insert(stateVar);
        }
    }
    // check whether the goal is satisfied
    for (int j = 0; j < htn->gSize; j++) {
        int stateVar = htn->gList[j];
        if (!this->stateSeq[i - 1].count(stateVar)) {
            this->executable = false;
            return;
        }
    }
    // convert the set-based representation of the state
    // sequence to the bit-based
    for (const unordered_set<int> &state : stateSeq) {
        boost::dynamic_bitset<> stateBits(htn->numStateBits);
        for (const int prop : state) {
            stateBits[prop] = true;
        }
        this->stateSeqBits.push_back(stateBits);
    }
}