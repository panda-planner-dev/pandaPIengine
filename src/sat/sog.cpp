#include <cassert>
#include <algorithm>
#include "sog.h"


SOG* runPOSOGOptimiser(SOG* sog, vector<int> & methods, Model* htn){
	vector<unordered_set<int>> antiAdj;
	vector<unordered_set<int>> antiBdj;
	// put the subtasks of the first method into the SOG
	sog->numberOfVertices = htn->numSubTasks[methods[0]];
	sog->labels.resize(sog->numberOfVertices);
	sog->adj.resize(sog->numberOfVertices);
	sog->bdj.resize(sog->numberOfVertices);
	antiAdj.resize(sog->numberOfVertices);
	antiBdj.resize(sog->numberOfVertices);
	vector<int> mapping;
	for (size_t i = 0; i < sog->numberOfVertices; i++){
		mapping.push_back(i);
		sog->labels[i].insert(htn->subTasks[methods[0]][i]);
	}
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
				sog->labels.push_back(_temp);
				sog->adj.push_back(_temp);
				sog->bdj.push_back(_temp);
				antiAdj.push_back(_temp);
				antiBdj.push_back(_temp);
			}


#ifndef NDEBUG
	cout << "Possible candidates " << matchingVertices.size() << endl;
#endif
			// TODO better do this with DP? At least in the total order case, this should be possible
			
			int myTask = htn->subTasks[m][subTask];
			int v = -1;
			for (int & x : matchingVertices)
				if (sog->labels[x].count(myTask)) {
					v = x;
					break;
				}
			if (v == -1) v = matchingVertices[0]; // just take the first vertex that fits
		
#ifndef NDEBUG
	cout << "Possible Vertices for " << htn->taskNames[myTask] << ":";
	for (int x : matchingVertices) cout << " " << x;
	cout << endl;
	cout << "Choosing: " << v << endl;
#endif

			mapping[subTask] = v;
			takenVertices.insert(v);
			sog->labels[v].insert(myTask);

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
	//assert(sog->numberOfVertices >= maxSize);
#endif

	return sog;
}


SOG* runTOSOGOptimiser(SOG* sog, vector<int> & methods, Model* htn){

	// TODO build adj, bdj
	sog->numberOfVertices = htn->numSubTasks[methods[0]];	
	sog->labels.resize(sog->numberOfVertices);
	sog->adj.resize(sog->numberOfVertices);
	sog->bdj.resize(sog->numberOfVertices);


	for (size_t mID = 0; mID < methods.size(); mID++){
		int m = methods[mID];
		// (x,y) -> minimum labels needed when putting the first x tasks to the first y positions
		vector<vector<int>> minAdditionalLabels (htn->numSubTasks[m]+1);
		vector<vector<bool>> optimalPutHere (htn->numSubTasks[m]+1);
		
		for (size_t i = 0; i <= htn->numSubTasks[m]; i++){
			minAdditionalLabels[i].resize(sog->numberOfVertices+1);
			optimalPutHere[i].resize(sog->numberOfVertices+1);
		}

		for (size_t x = 1; x <= htn->numSubTasks[m]; x++)
			minAdditionalLabels[x][0] = 1000*1000; // impossible
		for (size_t y = 1; y <= sog->numberOfVertices; y++)
			minAdditionalLabels[0][y] = 1000*1000; // impossible
		minAdditionalLabels[0][0] = 0;
		
		for (size_t x = 1; x <= htn->numSubTasks[m]; x++){
			int taskID = htn->methodTotalOrder[m][x-1];
			int task = htn->subTasks[m][taskID];
			for (size_t y = 1; y <= sog->numberOfVertices; y++){
				if (x > y) minAdditionalLabels[x][y] = 1000*1000; // impossible
				else {
					int labelNeeded = 1 - sog->labels[y-1].count(task);
					// put label here
					minAdditionalLabels[x][y] = labelNeeded + minAdditionalLabels[x-1][y-1];
					optimalPutHere[x][y] = true;

					if (minAdditionalLabels[x][y-1] < minAdditionalLabels[x][y]){
						minAdditionalLabels[x][y] = minAdditionalLabels[x][y-1];
						optimalPutHere[x][y] = false;
					}
				}
			}
		}

#ifndef NDEBUG
		cout << "For adding method " << m << " we need " << minAdditionalLabels[htn->numSubTasks[m]][sog->numberOfVertices] << " additional labels" << endl;
#endif	
		// get positions by recursion
		int x = htn->numSubTasks[m];
		int y = sog->numberOfVertices;

		vector<int> matching (htn->numSubTasks[m]);

		while (x != 0 && y != 0){
			if (optimalPutHere[x][y]){
				int taskID = htn->methodTotalOrder[m][x-1];
				int task = htn->subTasks[m][taskID];
#ifndef NDEBUG
				cout << "Putting task " << x << " " << htn->taskNames[task] << " @ " << y-1 << endl;
#endif	

				sog->labels[y-1].insert(task);
				matching[taskID] = y-1;
				x--;
				y--;
			} else {
				y--;
			}
		}
		sog->methodSubTasksToVertices.push_back(matching);
#ifndef NDEBUG
		cout << "Ended with x = " << x << " y = " << y << endl; 
#endif	
		assert(x == 0);

	}

	return sog;
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

	SOG* sog = new SOG;
	if (methods.size() == 0){
		sog->numberOfVertices = 0;
		return sog;
	}
	
	sort(methods.begin(), methods.end(), [&](int m1, int m2) {
        return htn->numSubTasks[m1] > htn->numSubTasks[m2];   
    });

#ifndef NDEBUG
	int maxSize = 0;
	for (size_t mID = 0; mID < methods.size(); mID++)
		if (maxSize < htn->numSubTasks[methods[mID]]) maxSize = htn->numSubTasks[methods[mID]];
	cout << endl << endl << endl << "Running PO SOG Optimiser with " << methods.size() << " methods with up to " << maxSize << " subtasks." << endl;
	//cout << htn->numSubTasks[methods[0]] << " max " << maxSize << endl;
#endif


	if (allMethodsAreTotallyOrdered)
		return runTOSOGOptimiser(sog, methods, htn);
	else
		return runPOSOGOptimiser(sog, methods, htn);
}
