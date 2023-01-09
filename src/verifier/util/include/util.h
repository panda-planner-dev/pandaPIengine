#ifndef _util_inc_h_
#define _util_inc_h_

#include "Model.h"
#include <boost/dynamic_bitset.hpp>

class Util {
    public:
        static bool isPrecondition(string name) {
            string prefix = "__method_precondition";
            string noop = "__noop";
            auto matching = mismatch(prefix.begin(), prefix.end(), name.begin());
            if (matching.first == prefix.end()) return true;
            auto matchingNoop = mismatch(noop.begin(), noop.end(), name.begin());
            if (matchingNoop.first == noop.end()) return true;
            return false;
        }
        
        static boost::dynamic_bitset<>* extractPrecondition(int action, Model *htn) {
            boost::dynamic_bitset<>* prec = new boost::dynamic_bitset<>(htn->numStateBits);
            for (int propIndex = 0; propIndex < htn->numPrecs[action]; propIndex++) {
                int prop = htn->precLists[action][propIndex];
                (*prec)[prop] = true;
            }
            return prec;
        }
};
#endif