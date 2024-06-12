#include "sat_verifier.h"
#include "state_formula.h"

bool SATVerifier::generateSATFormula(int depth, PDT *pdt) {
    string prefix;
    std::clock_t before, after;
    double elapsed;
    long numVariables, numVarsBefore, numVarsAfter;
    long numClauses, numClausesBefore, numClausesAfter;

    prefix = "[Generating the PDT] ";
    cout << prefix << "Depth: " << to_string(depth) << endl;
    before = std::clock();
    pdt->expandPDTUpToLevel(depth, this->htn, false);
    after = std::clock();
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;
    pdt->resetPruning(this->htn);

    prefix = "[Extracting the SOG] ";
    before = std::clock();
    vector<PDT*> leafs;
    pdt->getLeafs(leafs);
    SOG *sog = pdt->getLeafSOG();
    after = std::clock();
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
#ifndef NDEBUG
    this->sog = sog;
#endif
    int numVertices = sog->numberOfVertices;
    cout << prefix << "Num vertices: " << to_string(numVertices) << endl;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;
    if (numVertices < this->plan.size()) return false;

    prefix = "[Generating variables for the PDT] ";
    numVarsBefore = capsule.number_of_variables;
    before = std::clock();
    pdt->assignVariableIDs(this->capsule, this->htn);
    after = std::clock();
    numVarsAfter = capsule.number_of_variables;
    numVariables = numVarsAfter - numVarsBefore;
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
    cout << prefix << "Num of variables: " << to_string(numVariables) << endl;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;

    prefix = "[Generating clauses for decomposition] ";
    numClausesBefore = get_number_of_clauses();
    before = std::clock();
    assertYes(solver,pdt->abstractVariable[0]);
    pdt->addDecompositionClauses(this->solver, this->capsule, this->htn);
    no_abstract_in_leaf(this->solver, leafs, this->htn);
    after = std::clock();
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
    numClausesAfter = get_number_of_clauses();
    numClauses = numClausesAfter - numClausesBefore;
    cout << prefix << "Num clauses: " << to_string(numClauses) << endl;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;

    prefix = "[Generating state variables] ";
    numVarsBefore = this->capsule.number_of_variables;
    before = std::clock();
    StateVariables *stateVars = new StateVariables(this->plan,
                                                   this->htn,
                                                   this->capsule);
    after = std::clock();
    numVarsAfter = this->capsule.number_of_variables;
    numVariables = numVarsAfter - numVarsBefore;
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
    cout << prefix << "Num variables: " << to_string(numVariables) << endl;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;

    prefix = "[Generating mapping variables] ";
    numVarsBefore = this->capsule.number_of_variables;
    before = std::clock();
    PlanToSOGVars *mapping = new PlanToSOGVars(this->plan,
                                               sog,
                                               this->htn,
                                               this->capsule);
    after = std::clock();
#ifndef NDEBUG
    this->mapping = mapping;
#endif
    numVarsAfter = this->capsule.number_of_variables;
    numVariables = numVarsAfter - numVarsBefore;
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
    cout << prefix << "Num variables: " << to_string(numVariables) << endl;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;

    prefix = "[Generating constraints on states] ";
    numClausesBefore = get_number_of_clauses();
    before = std::clock();
    ConstraintsOnStates(this->solver,
                        this->htn,
                        stateVars,
                        this->execution);
    after = std::clock();
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
    numClausesAfter = get_number_of_clauses();
    numClauses = numClausesAfter - numClausesBefore;
    cout << prefix << "Num clauses: " << to_string(numClauses) << endl;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;

    prefix = "[Generating constraints on mapping] ";
    numClausesBefore = get_number_of_clauses();
    before = std::clock();
    ConstraintsOnMapping(this->solver,
                         this->capsule,
                         this->plan,
                         mapping,
                         sog);
    after = std::clock();
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
    numClausesAfter = get_number_of_clauses();
    numClauses = numClausesAfter - numClausesBefore;
    cout << prefix << "Num clauses: " << to_string(numClauses) << endl;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;

    prefix = "[Generating constraints on plan execution] ";
    numClausesBefore = get_number_of_clauses();
    before = std::clock();
    ConstraintsOnSequence(this->solver,
                          this->htn,
                          this->plan.size(),
                          sog,
                          stateVars,
                          mapping);
    after = std::clock();
    elapsed = 1000.0 * (after - before) / CLOCKS_PER_SEC;
    numClausesAfter = get_number_of_clauses();
    numClauses = numClausesAfter - numClausesBefore;
    cout << prefix << "Num clauses: " << to_string(numClauses) << endl;
    cout << prefix << "Done! Time: " << to_string(elapsed) << endl;
    return true;
}

#ifndef NDEBUG
void SATVerifier::print(void *solver, PDT *pdt) {
    int currentID = 0;
    vector<PDT*> leafs;
    pdt->getLeafs(leafs);
    pdt->assignOutputNumbers(solver, currentID, this->htn);
    cout << "- Mappings between the plan and the leafs" << endl;
    for (int pos = 0; pos < this->plan.size(); pos++) {
        for (int v = 0; v < this->sog->numberOfVertices; v++) {
            PDT *leaf = sog->leafOfNode[v];
            int posToVertex = this->mapping->getPosToVertexVar(pos, v);
            int prim = -1;
            if (ipasir_val(solver, posToVertex) > 0) {
                for (int i = 0; i < leaf->possiblePrimitives.size(); i++) {
                    prim = leaf->possiblePrimitives[i];
                    if (prim == this->plan[pos]) {
                        int var = leaf->primitiveVariable[i];
                        assert(ipasir_val(solver, var) > 0);
                        break;
                    }
                }
                assert(prim == this->plan[pos]);
                int outID = leaf->outputID;
                string name = this->htn->taskNames[prim];
                cout << pos << "->" << v << "|" << outID << " " << name << endl;
            }
            int vertexToPos = this->mapping->getVertexToPosVar(v, pos);
            if (ipasir_val(solver, vertexToPos) > 0) {
                for (int i = 0; i < leaf->possiblePrimitives.size(); i++) {
                    prim = leaf->possiblePrimitives[i];
                    int var = leaf->primitiveVariable[i];
                    if (ipasir_val(solver, var) > 0) {
                        int outID = leaf->outputID;
                        string name = this->htn->taskNames[prim];
                        cout << pos << "<-" << v << "|" << outID << " " << name << endl;
                    }
                }
            }
        }
    }
    cout << "- Decomposition hierarchy" << endl;
    pdt->printDecomposition(this->htn);
}
#endif