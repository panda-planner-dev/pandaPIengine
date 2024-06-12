#ifndef _execution_inc_h_
#define _execution_inc_h_

#include "Model.h"
#include <boost/dynamic_bitset.hpp>

class PlanExecution {
    public:
        PlanExecution(Model *htn, vector<int> plan);
        unordered_set<int> getState(int pos) {return this->stateSeq[pos];}
        boost::dynamic_bitset<> getStateBits(int pos) {return stateSeqBits[pos];}
        bool isPropTrue(int pos, int prop) {return stateSeq[pos].count(prop);}
        bool isExecutable() const {return executable;}
        int getStateSeqLen() const {return stateSeq.size();}

    private:
        bool executable;
        vector<unordered_set<int>> stateSeq;
        vector<boost::dynamic_bitset<>> stateSeqBits;
};
#endif