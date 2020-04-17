#include <cassert>
#include "sog.h"


SOG* runPOSOGOptimiser(vector<int> & methods, Model* htn){
	SOG* sog = new SOG;
	if (methods.size() == 0){
		sog->numberOfVertices = 0;
		return sog;
	}

#ifndef NDEBUG
	int maxSize = 0;
	for (size_t mID = 0; mID < methods.size(); mID++)
		if (maxSize < htn->numSubTasks[methods[mID]]) maxSize = htn->numSubTasks[methods[mID]];
	//cout << "Running PO SOG Optimiser with " << methods.size() << " methods with up to " << maxSize << " subtasks." << endl;
#endif

	vector<unordered_set<int>> antiAdj;
	vector<unordered_set<int>> antiBdj;
	// put the subtasks of the first method into the SOG
	sog->numberOfVertices = htn->numSubTasks[methods[0]];
	sog->adj.resize(sog->numberOfVertices);
	sog->bdj.resize(sog->numberOfVertices);
	antiAdj.resize(sog->numberOfVertices);
	antiBdj.resize(sog->numberOfVertices);
	vector<int> mapping;
	for (size_t i = 0; i < sog->numberOfVertices; i++)
		mapping.push_back(i);
	sog-> methodSubTasksToVertices.push_back(mapping);


	// add ordering
	for (int ordering = 0; ordering < htn->numOrderings[methods[0]]; ordering += 2){
		sog->adj[htn->ordering[methods[0]][ordering]].insert(htn->ordering[methods[0]][ordering + 1]);
		sog->bdj[htn->ordering[methods[0]][ordering + 1]].insert(htn->ordering[methods[0]][ordering]);
	}
	// compute complement of order set
	for (int i = 0; i < sog->numberOfVertices; i++)
		for (int j = 0; j < sog->numberOfVertices; j++){
			if (i == j) continue;
			if (sog->adj[i].count(j)) continue;
			antiAdj[i].insert(j);
			antiBdj[j].insert(i);
		}
	
	// merge the vertices of the other methods
	for (size_t mID = 1; mID < methods.size(); mID++){
		int m = methods[mID];
		mapping.clear();
		mapping.resize(htn->numSubTasks[m]);
		unordered_set<int> takenVertices;

		for (size_t stID = 0; stID < htn->numSubTasks[m]; stID++){
			int subTask = htn->methodTotalOrder[m][stID]; // use positions in topological order, may lead to better results

			// try to find a matching task
		
			vector<int> matchingVertices;
			for (size_t v = 0; v < sog->numberOfVertices; v++){
				if (takenVertices.count(v)) continue;
				// check all previous tasks of this method
				for (size_t ostID = 0; ostID < stID; ostID++){
					int otherSubTask = htn->methodTotalOrder[m][ostID];
					int ov = mapping[otherSubTask]; // vertex of other task
					
					if (htn->methodSubTasksSuccessors[m][subTask].count(otherSubTask)){
						if (sog->bdj[v].count(ov)) goto next_vertex;
						if (antiAdj[v].count(ov)) goto next_vertex;
					} else if (htn->methodSubTasksPredecessors[m][subTask].count(otherSubTask)){
						
						if (sog->adj[v].count(ov)) goto next_vertex;
						if (antiBdj[v].count(ov)) goto next_vertex;
					} else {
						// no order is allowed to be present
						if (sog->adj[v].count(ov)) goto next_vertex;
						if (sog->bdj[v].count(ov)) goto next_vertex;
					}

				}
			
				// if we reached this point, this vertex is ok
				matchingVertices.push_back(v);
next_vertex:;
			}

			if (matchingVertices.size() == 0){
				matchingVertices.push_back(sog->numberOfVertices);
				// add the vertex
				sog->numberOfVertices++;
				unordered_set<int> _temp;
				sog->adj.push_back(_temp);
				sog->bdj.push_back(_temp);
				antiAdj.push_back(_temp);
				antiBdj.push_back(_temp);
			}

			// TODO better selection
			int v = matchingVertices[0]; // just take the first vertex that fits
			mapping[subTask] = v;
			takenVertices.insert(v);

			// add the vertices and anti-vertices needed for this new node
			for (size_t ostID = 0; ostID < stID; ostID++){
				int otherSubTask = htn->methodTotalOrder[m][ostID];
				int ov = mapping[otherSubTask]; // vertex of other task
				
				if (htn->methodSubTasksSuccessors[m][subTask].count(otherSubTask)){
					sog->adj[v].insert(ov);
					antiAdj[ov].insert(v);
					sog->bdj[ov].insert(v);
					antiBdj[v].insert(ov);
				} else if (htn->methodSubTasksPredecessors[m][subTask].count(otherSubTask)){
					sog->adj[ov].insert(v);
					antiAdj[v].insert(ov);
					sog->bdj[v].insert(ov);
					antiBdj[ov].insert(v);
				} else {
					antiAdj[v].insert(ov);
					antiAdj[ov].insert(v);
					antiBdj[v].insert(ov);
					antiBdj[ov].insert(v);
				}
			}
		}
		sog->methodSubTasksToVertices.push_back(mapping);
	}

#ifndef NDEBUG
	assert(sog->numberOfVertices == maxSize);
#endif

	return sog;
}


SOG* runTOSOGOptimiser(vector<int> & methods, Model* htn){
	// TODO implement TO optimiser
	return runPOSOGOptimiser(methods,htn);
}


// just the greedy optimiser
SOG* optimiseSOG(vector<int> & methods, Model* htn){
	// edge case
	bool allMethodsAreTotallyOrdered = true;
	for (const int & m : methods)
		if (!htn->methodIsTotallyOrdered[m]){
			allMethodsAreTotallyOrdered = false;
			break;
		}

	if (allMethodsAreTotallyOrdered)
		return runTOSOGOptimiser(methods, htn);
	else
		return runPOSOGOptimiser(methods, htn);
}
