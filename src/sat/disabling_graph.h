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
};



void compute_disabling_graph(Model * htn);

#endif
