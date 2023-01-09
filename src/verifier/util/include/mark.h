#ifndef _mark_inc_h_
#define _mark_inc_h_

#include "Model.h"
#include "execution.h"
#include <boost/dynamic_bitset.hpp>

class MethodPrecMarker {
    public:
        MethodPrecMarker(Model *htn);
        bool isMarked(int task) {return this->marked[task];}
        bool isMemorized(int task) {return this->memorized[task];}
        vector<boost::dynamic_bitset<>*> getProps(int task) {return this->props[task];}
        bool isMethodPrecSat(int t, int pos, PlanExecution *execution);
         
    private:
        Model *htn;
        vector<bool> marked;
        vector<bool> memorized;
        vector<vector<boost::dynamic_bitset<>*>> props;

        void dfs(int m, vector<bool> &visited);
};
#endif