//
// Created by u6162630 on 4/3/23.
//

#include "constraints.h"
ConstraintsOnStates::ConstraintsOnStates(
        void *solver,
        Model *htn,
        StateVariables *vars,
        PlanExecution *execution) {
    int length = execution->getStateSeqLen();
    int numProps = htn->numStateBits;
    for (int pos = 0; pos < length - 1; pos++) {
        for (int prop = 0; prop < numProps; prop++) {
            int var = vars->get(pos, prop);
            if (execution->isPropTrue(pos, prop)) {
                assertYes(solver, var);
            } else {assertNot(solver, var);}
        }
    }
}

ConstraintsOnMapping::ConstraintsOnMapping(
        void *solver,
        sat_capsule &capsule,
        vector<int> &plan,
        PlanToSOGVars *mapping,
        SOG *sog) {
    vector<vector<int>> mappingPerVertex(sog->numberOfVertices);
    for (int pos = 0; pos < plan.size(); pos++) {
        // get the variable indicating whether
        // the plan step is matched to a vertex
        int matchedVar = mapping->getMatchedVar(pos);
        // every plan step must match to some vertex
        assertYes(solver, matchedVar);
        vector<int> possibleMappings;
        for (int v = 0; v < sog->numberOfVertices; v++){
            // get the variable representing the mapping
            // from a plan step to a vertex
            int posToVertex = mapping->getPosToVertexVar(pos, v);
            int taskVar = mapping->getTaskVar(pos, v);
            if (taskVar == -1) {
                assertNot(solver, posToVertex);
            } else {
                implies(solver, posToVertex, taskVar);
                possibleMappings.push_back(posToVertex);
            }
            // get the variable representing the mapping
            // from the vertex to the plan step
            int vertexToPos = mapping->getVertexToPosVar(v, pos);
            if (!mapping->vertexHasArtiPrim(v)) {
                // if the vertex does not have any artificial
                // action, then the mapping from the vertex
                // to the position cannot be activated
                assertNot(solver, vertexToPos);
            } else {
                vector<int> vars;
                vector<int>::iterator iter;
                // iterate through all artificial actions in the vertex
                for (iter = mapping->abegin(v); iter < mapping->aend(v); iter++) {
                    int var = mapping->getArtificialVar(v, *iter);
                    assert(var != -1);
                    vars.push_back(var);
                    int varAtSeq = mapping->getSequenceVar(pos, *iter);
                    // if the vertex is mapped to the position and the artificial
                    // action is activated, then the respective action in the
                    // position should also be activated
                    impliesAnd(solver, vertexToPos, var, varAtSeq);
                }
                // if the mapping from the vertex to the position
                // is activated, then some artificial action in the
                // vertex should also be activated
                impliesOr(solver, vertexToPos, vars);
            }
            mappingPerVertex[v].push_back(posToVertex);
            mappingPerVertex[v].push_back(vertexToPos);
            // get the variable representing that the mapping
            // between the position and the vertex is forbidden
            int forbiddenVar = mapping->getForbiddenVar(pos, v);
            // if the mapping is forbidden, then neither the mapping
            // from the position to the vertex nor the mapping from
            // the vertex to the position is allowed
            impliesNot(solver, forbiddenVar, posToVertex);
            impliesNot(solver, forbiddenVar, vertexToPos);
            // if the mapping between the position and the vertex
            // exists, then all the successors of the vertex are
            // forbidden to be mapped to the previous position
            for (const int successor : sog->adj[v]) {
                int forbidNext = mapping->getForbiddenVar(
                        pos, successor);
                implies(solver, forbiddenVar, forbidNext);
                if (pos == 0) break;
                int forbidPrevNext = mapping->getForbiddenVar(
                        pos - 1, successor);
                implies(solver,
                        posToVertex, forbidPrevNext);
                implies(solver,
                        vertexToPos, forbidPrevNext);
            }
            if (pos == 0) continue;
            int forbiddenPrev = mapping->getForbiddenVar(
                    pos - 1, v);
            implies(solver, forbiddenVar, forbiddenPrev);
        }
        // every plan step can be mapped to at most one
        // vertex that has the respective action
        atMostOne(solver, capsule, possibleMappings);
        // if the plan step is matched, then it must be mapped
        // to some vertex that has the respective action
        impliesOr(solver, matchedVar, possibleMappings);
    }
    for (int v = 0; v < sog->numberOfVertices; v++) {
        atMostOne(solver, capsule, mappingPerVertex[v]);
        int activatedVar = mapping->getActivatedVar(v);
        // if the vertex is activated, it must be mapped
        // to some plan step
        impliesOr(solver, activatedVar, mappingPerVertex[v]);
        vector<int> primVars;
        PDT *pdt = sog->leafOfNode[v];
        for (int i = 0; i < pdt->possiblePrimitives.size(); i++) {
            int primVar = pdt->primitiveVariable[i];
            primVars.push_back(primVar);
        }
        // if the vertex is not activated, then all actions
        // in this vertex cannot be activated
        // impliesOr(solver, activatedVar, primVars);
        notImpliesAllNot(solver, activatedVar, primVars);
    }
}