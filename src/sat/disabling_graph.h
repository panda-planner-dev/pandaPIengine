#ifndef disabling_graph_INCLUDED
#define disabling_graph_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"
#include "pdt.h"


struct graph{
	int numVertices;
	int** adj;
	int* adjSize;

	graph(int num);
	graph(vector<unordered_set<int>> & tempAdj);
	~graph();

	int count_edges();
	void calcSCCs();
	void calcSCCGraph();
	bool can_reach_any_of(vector<int> & from, vector<int> & to);
	bool can_reach_any_of_directly(vector<int> & from, vector<int> & to);

	graph * scc_graph;

	// place to keep the SCC data
	int numSCCs = -1;
	int* vertexToSCC = nullptr;
	int** sccToVertices = nullptr;
	int* sccSize = nullptr;
	

	string dot_string();
	string dot_string(map<int,string> names);
	string dot_string(map<int,string> names, map<int,string> nodestyles);

private:
	void tarjan(int v);
};

graph * compute_disabling_graph(Model * htn, bool no_invariant_inference);

vector<vector<int>> compute_block_compression(Model * htn, vector<PDT*> & leafs);


#endif
