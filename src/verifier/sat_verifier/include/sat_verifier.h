#include "Model.h"
#include "sog.h"
#include "pdt.h"
#include "ipasir.h"
#include "verifier.h"
#include "depth.h"
#include "execution.h"
#include "variables.h"
#include "constraints.h"
#include "sat_encoder.h"

class SATVerifier : public Verifier {
    public:
        SATVerifier(
                string htnFile,
                string planFile,
                bool optimizeDepth=false) : Verifier(htnFile, planFile) {
            this->execution = new PlanExecution(this->htn, this->plan);
            this->htn->computeTransitiveClosureOfMethodOrderings();
            this->htn->buildOrderingDatastructures();
            this->htn->isTotallyOrdered = false;
            if (!this->execution->isExecutable()) {
                this->result = false;
                return;
            }
            PDT *pdt = new PDT(this->htn);
            int maxDepth = 2 * (this->plan.size()) * (this->htn->numTasks - this->htn->numActions);
            if (optimizeDepth) {
                string prefix = "[Computing optimal depth]";
                Depth *depth = new Depth(
                        this->htn,
                        this->plan.size());
                int optimalDepth = depth->get();
#ifndef NDEBUG
                if (optimalDepth > maxDepth)
                    cout << prefix << " Higher optimal depth!" << endl;
#endif
                maxDepth = min(maxDepth, optimalDepth);
                cout << prefix << " Max depth: " << maxDepth << endl;
            }
            for (int depth = 1; depth <= maxDepth; depth++) {
                this->solver = ipasir_init();
                int state = 20;
                if (this->generateSATFormula(depth, pdt)) {
                    string prefix = "[Solving the SAT formula] ";
                    std::clock_t before = std::clock();
                    state = ipasir_solve(this->solver);
                    std::clock_t after = std::clock();
                    double elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
                    if (state == 10) {
                        cout << prefix << "Solved!" << endl;
                        this->result = true;
                        cout << prefix << "Time: " << to_string(elapsed) << endl;
#ifndef NDEBUG
                        this->print(solver, pdt);
#endif
                        ipasir_release(this->solver);
                        return;
                    } else {cout << prefix << "Unsolvable" << endl;}
                    cout << prefix << "Time: " << to_string(elapsed) << endl;
                }
                ipasir_release(this->solver);
            }
        }
    
    private:
        sat_capsule capsule;
        PlanExecution *execution;
        void *solver;

#ifndef NDEBUG
        PlanToSOGVars *mapping;
        SOG *sog;
#endif

        bool generateSATFormula(int depth, PDT *pdt);

#ifndef NDEBUG
        void print(void *solver, PDT *pdt);
#endif
};