#ifndef _VARIABLES_H_
#define _VARIABLES_H_
#include "Model.h"
#include "execution.h"
#include "sat_encoder.h"
#include "sog.h"
#include "pdt.h"

class StateVariables {
public:
    StateVariables(
            vector<int> &plan,
            Model *htn,
            sat_capsule &capsule) {
        for (int pos = 0; pos < plan.size(); pos++) {
            vector<int> varsPerPos;
            int total = htn->numStateBits;
            for (int prop = 0; prop < total; prop++) {
                int v = capsule.new_variable();
                string propName = htn->factStrs[prop];
                string varName = propName + "@" + to_string(pos);
                DEBUG(capsule.registerVariable(v, varName));
                varsPerPos.push_back(v);
            }
            this->vars.push_back(varsPerPos);
        }
    }
    int get(int pos, int prop) {return this->vars[pos][prop];}
    vector<int>::iterator begin(int pos) {return this->vars[pos].begin();}
    vector<int>::iterator end(int pos) {return this->vars[pos].end();}
private:
    vector<vector<int>> vars;
};

class PlanToSOGVars {
public:
    PlanToSOGVars(
            vector<int> &plan,
            SOG *sog,
            Model *htn,
            sat_capsule &capsule);
    int getMatchedVar(int pos) {return this->matched[pos];}
    int getPosToVertexVar(int pos, int v) {return this->posToVertex[pos][v];}
    int getVertexToPosVar(int v, int pos) {return this->vertexToPos[v][pos];}
    int getForbiddenVar(int pos, int v) {return this->forbidden[pos][v];}
    int getTaskVar(int pos, int v) {return this->tasks[pos][v];}
    int getActivatedVar(int v) {return this->activated[v];}
    int getSequenceVar(int pos, int prim) {return this->sequence[pos][prim];}
    bool vertexHasArtiPrim(int v) {return this->hasArtiPrim[v];}
    vector<int>::iterator abegin(int v) {return this->artificialPrims[v].begin();}
    vector<int>::iterator aend(int v) {return this->artificialPrims[v].end();}
    int getArtificialVar(int v, int action) {return this->artificial[v][action];}

private:
    vector<int> matched;
    vector<vector<int>> posToVertex;
    vector<vector<int>> vertexToPos;
    vector<vector<int>> forbidden;
    vector<vector<int>> tasks;
    vector<int> activated;
    vector<vector<int>> artificial;
    vector<vector<int>> artificialPrims;
    vector<vector<int>> sequence;
    vector<bool> hasArtiPrim;
};
#endif