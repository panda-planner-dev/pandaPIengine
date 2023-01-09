#include "verifier.h"
#include "execution.h"
#include "graph.h"
#include "mapping.h"
#include "mark.h"
#include "method_graph.h"

class TOVerifier : public Verifier {
    public:
        TOVerifier(string htnFile, string planFile) : Verifier(htnFile, planFile) {
            this->initialize();
            this->result = this->verify();
        }


    private:
        PlanExecution *execution;
        InverseMapping *invMapping;
        MethodPrecMarker *precMarker;
        MethodGraph *g;

        // the CYK table
        vector<vector<unordered_set<int>>> table;

        bool is2NormForm() {
            for (int i = 0; i < this->htn->numMethods; i++) {
                int numSubTask = this->htn->numSubTasks[i];
                if (numSubTask > 2) return false;
            }
            return true;
        }

        bool verify() {
            // Just like CYK, the idea is to construct a 2-dim table 
            // where a solt A[i, j] contains all possible (compound)
            // tasks which can be decomposed into the subsequence
            // plan[i, j] of the given plan 
            PlanExecution *execution = new PlanExecution(this->htn, this->plan);
            if (!execution->isExecutable()) {
                cout << "The plan is not executable" << endl;
                return false;
            }
            int dim = this->plan.size();
            this->table.resize(dim);
            for (int start = dim - 1; start >= 0; start--) {
                this->table[start].resize(dim);
                // fill in the table
                for (size_t end = start; end < dim; end++) {
                    updateTable(start, end);
#ifndef NDEBUG
                    cout << "Subsequence " << start << " to " << end << " includes tasks:" << endl;
                    for (const auto &t : this->table[start][end]) {
                        cout << "\t" << this->htn->taskNames[t] << endl;
                    }
#endif
                }
            }
            return this->table[0][dim-1].count(this->htn->initialTask);
        }

        void initialize() {
            if (!this->is2NormForm()) {
                cout << "Unable to initialize the verifier due to unsupport model format" << endl;
                exit(-1);
            }
            this->precMarker = new MethodPrecMarker(this->htn);
            this->invMapping = new InverseMapping(this->htn, this->precMarker);
            this->g = new MethodGraph(this->htn, this->precMarker);
            this->execution = new PlanExecution(this->htn, this->plan);
        }

        void updateTable(int start, int end);
        void dfs(int m, int start, int end, unordered_set<int> &visited, unordered_set<int> &validiated);
};