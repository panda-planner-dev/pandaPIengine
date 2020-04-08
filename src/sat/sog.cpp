#include "sog.h"


// just the greedy optimiser
SOG* optimiseSOG(vector<int> & methods, Model* htn){
	SOG* sog = new SOG;
	if (methods.size() == 0){
		sog->numberOfVertices = 0;
		return sog;
	}

	// TODO: method orderings must be transitively closed

	vector<set<int>> antiAdj;
	// put the subtasks of the first method into the SOG
	sog->numberOfVertices = htn->numSubTasks[methods[0]];
	sog->adj.resize(sog->numberOfVertices);
	for (int ordering = 0; ordering < htn->numOrderings[methods[0]]; ordering += 2){
		sog->adj[htn->ordering[methods[0]][ordering]].insert(htn->ordering[methods[0]][ordering + 1]);
	}
	// compute complement of order set
	for (int i = 0; i < sog->numberOfVertices; i++)
		for (int j = 0; j < sog->numberOfVertices; j++){
			if (i == j) continue;
			if (sog->adj[i].count(j)) continue;
			antiAdj[i].insert(j);
		}




	return sog;
}

