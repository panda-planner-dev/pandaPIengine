#ifndef disabling_graph_INCLUDED
#define disabling_graph_INCLUDED

#include "../flags.h" // defines flags
#include "../Model.h"


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

	graph * scc_graph;

	// place to keep the SCC data
	int numSCCs = -1;
	int* vertexToSCC = nullptr;
	int** sccToVertices = nullptr;
	int* sccSize = nullptr;
	

	string dot_string();
	string dot_string(map<int,string> names);

private:
	void tarjan(int v);
};



void compute_disabling_graph(Model * htn);

#endif
