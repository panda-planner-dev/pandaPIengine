#ifndef _graph_inc_h_
#define _graph_inc_h_

#include <unordered_set>
#include <vector>
#include "assert.h"

using namespace std;

class DirectedGraph{
    public:
        DirectedGraph(int n) {
            this->V = n;
            this->edges.resize(this->V);
        }
        bool isValidVertex(int v) {return v < this->V && v >= 0;}
        void connect(int u, int v) {
            assert(u < this->V && v < this->V);
            this->edges[u].insert(v);
        }
        unordered_set<int>::iterator adjBegin(int v) {return this->edges[v].begin();}
        unordered_set<int>::iterator adjEnd(int v) {return this->edges[v].end();}
    
    private:
        int V; // num of vertices
        vector<unordered_set<int>> edges;
};
#endif