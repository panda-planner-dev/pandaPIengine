#include "variables.h"
#include "util.h"

PlanToSOGVars::PlanToSOGVars(
        vector<int> &plan,
        SOG *sog,
        Model *htn,
        sat_capsule &capsule) {
    int var;
    string name;
    this->artificial.resize(sog->numberOfVertices);
    this->artificialPrims.resize(sog->numberOfVertices);
    this->hasArtiPrim.assign(sog->numberOfVertices, false);
    for (int v = 0; v < sog->numberOfVertices; v++) {
        this->artificial[v].assign(htn->numActions, -1);
        PDT *pdt = sog->leafOfNode[v];
        for (int i = 0; i < pdt->possiblePrimitives.size(); i++) {
            int action = pdt->possiblePrimitives[i];
            string actionName = htn->taskNames[action];
            if (Util::isPrecondition(actionName)) {
                var = pdt->primitiveVariable[i];
                this->artificial[v][action] = var;
                this->artificialPrims[v].push_back(action);
                this->hasArtiPrim[v] = true;
            }
        }
        var = capsule.new_variable();
        name = "activated[" + to_string(v) + "]";
        DEBUG(capsule.registerVariable(var, name));
        this->activated.push_back(var);
    }
    assert(this->activated.size() == sog->numberOfVertices);
    this->vertexToPos.resize(sog->numberOfVertices);
    this->sequence.resize(plan.size());
    for (int pos = 0; pos < plan.size(); pos++) {
        int p = plan[pos];
        vector<int> mappingPerPos;
        mappingPerPos.assign(sog->numberOfVertices, -1);
        vector<int> forbiddenPerPos;
        forbiddenPerPos.assign(sog->numberOfVertices, -1);
        vector<int> taskPerPos;
        taskPerPos.assign(sog->numberOfVertices, -1);
        this->sequence[pos].assign(htn->numActions, -1);

        var = capsule.new_variable();
        name = "matched[" + to_string(pos) + "]";
        DEBUG(capsule.registerVariable(var, name));
        this->matched.push_back(var);

        for (int v = 0; v < sog->numberOfVertices; v++) {
            for (const int prim : this->artificialPrims[v]) {
                if (this->sequence[pos][v] != -1) continue;
                var = capsule.new_variable();
                name = htn->taskNames[prim] + "@" + to_string(pos);
                DEBUG(capsule.registerVariable(var, name));
                this->sequence[pos][prim] = var;
            }
            PDT *pdt = sog->leafOfNode[v];
            for (size_t t = 0; t < pdt->possiblePrimitives.size(); t++) {
                int action = pdt->possiblePrimitives[t];
                string actionName = htn->taskNames[action];
                if (action == p) {
                    taskPerPos[v] = pdt->primitiveVariable[t];
                    break;
                }
            }
            var = capsule.new_variable();
            name = "vertex_pos[" + to_string(v) + ";" + to_string(pos) + "]";
            DEBUG(capsule.registerVariable(var, name));
            this->vertexToPos[v].push_back(var);

            var = capsule.new_variable();
            name = "pos_vertex[" + to_string(pos) + ";" + to_string(v) + "]";
            DEBUG(capsule.registerVariable(var, name));
            mappingPerPos[v] = var;

            var = capsule.new_variable();
            name = "forbidden[" + to_string(pos) + ";" + to_string(v) + "]";
            DEBUG(capsule.registerVariable(var, name));
            forbiddenPerPos[v] = var;
        }
        this->posToVertex.push_back(mappingPerPos);
        this->forbidden.push_back(forbiddenPerPos);
        this->tasks.push_back(taskPerPos);
    }
}