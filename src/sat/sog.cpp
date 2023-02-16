#include <cassert>
#include <algorithm>
#include <iomanip>
#include "sog.h"
#include "pdt.h"


SOG* runPOSOGOptimiser(SOG* sog, vector<tuple<uint32_t,uint32_t,uint32_t>> & methods, Model* htn, bool effectLessActionsInSeparateLeaf){
	vector<unordered_set<uint16_t>> antiAdj;
	vector<unordered_set<uint16_t>> antiBdj;
	int m0 = get<0>(methods[0]);
	// put the subtasks of the first method into the SOG
	sog->numberOfVertices = htn->numSubTasks[m0];
	sog->labels.resize(sog->numberOfVertices);
	sog->adj.resize(sog->numberOfVertices);
	sog->bdj.resize(sog->numberOfVertices);
	antiAdj.resize(sog->numberOfVertices);
	antiBdj.resize(sog->numberOfVertices);
	vector<uint16_t> mapping;
	unordered_set<int> vertices_for_effectless_actions;
	for (size_t i = 0; i < sog->numberOfVertices; i++){
		mapping.push_back(i);
		int task = htn->subTasks[m0][i];
		sog->labels[i].insert(task);
		if (task < htn->numActions && htn->numAdds[task] == 0 && htn->numDels[task] == 0) vertices_for_effectless_actions.insert(i);
	}
	sog->methodSubTasksToVertices.push_back(mapping);



	// add ordering
	for (int ordering = 0; ordering < htn->numOrderings[m0]; ordering += 2){
		sog->adj[htn->ordering[m0][ordering]].insert(htn->ordering[m0][ordering + 1]);
		sog->bdj[htn->ordering[m0][ordering + 1]].insert(htn->ordering[m0][ordering]);
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
		int m = get<0>(methods[mID]);
		mapping.clear();
		mapping.resize(htn->numSubTasks[m]);
		unordered_set<int> takenVertices;

		for (size_t stID = 0; stID < htn->numSubTasks[m]; stID++){
			int subTask = htn->methodTotalOrder[m][stID]; // use positions in topological order, may lead to better results
			
			int myTask = htn->subTasks[m][subTask];
			bool isEffectless = false;
			if (myTask < htn->numActions && (htn->numAdds[myTask] == 0 && htn->numDels[myTask] == 0)) isEffectless = true;
				
			// try to find a matching task
			vector<int> matchingVertices;
			for (size_t v = 0; v < sog->numberOfVertices; v++){
				if (takenVertices.count(v)) continue;
				// if enabled, effect-less actions may only be grouped together with other effect-less actions
				if (effectLessActionsInSeparateLeaf && isEffectless != vertices_for_effectless_actions.count(v)) continue;
				
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
				unordered_set<uint16_t> _temp16;
				unordered_set<uint32_t> _temp32;
				sog->labels.push_back(_temp32);
				sog->adj.push_back(_temp16);
				sog->bdj.push_back(_temp16);
				antiAdj.push_back(_temp16);
				antiBdj.push_back(_temp16);
			}


#ifndef NDEBUG
	cout << "Possible candidates " << matchingVertices.size() << endl;
#endif
			// TODO better do this with DP? At least in the total order case, this should be possible
			
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
#ifndef NDEBUG
			cout << "-----------------------------------" << endl;
			set<int> types;
			for (int x : sog->labels[v])
				cout << "\t" << htn->taskNames[x] << " " << (x < htn->numActions && (htn->numAdds[x] == 0 && htn->numDels[x] == 0))<< endl,
					 types.insert((x < htn->numActions && (htn->numAdds[x] == 0 && htn->numDels[x] == 0)));
			cout << "++++" << htn->taskNames[myTask]  << " " << (myTask < htn->numActions && (htn->numAdds[myTask] == 0 && htn->numDels[myTask] == 0)) << endl;
			cout << "-----------------------------------" << endl;
			if (types.size() && types.count(myTask < htn->numActions && (htn->numAdds[myTask] == 0 && htn->numDels[myTask] == 0)) == 0)
				exit(0);
#endif


			sog->labels[v].insert(myTask);
			if (myTask < htn->numActions && (htn->numAdds[myTask] == 0 && htn->numDels[myTask] == 0)) vertices_for_effectless_actions.insert(v);

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

void addSequenceToSOG(SOG * sog, vector<int> & sequence, vector<int> & matchingPositions,
		int sequenceID, int begin, int end) {
	int targetLength = end - begin;
	//cout << "OPTI " << end << " - " << begin << " = " << targetLength << endl;

	// (x,y) -> minimum labels needed when putting the first x tasks to the first y positions
	vector<vector<int>> minAdditionalLabels (sequence.size() + 1);
	vector<vector<bool>> optimalPutHere (sequence.size() + 1);
	
	for (size_t i = 0; i <= sequence.size(); i++){
		minAdditionalLabels[i].resize(targetLength + 1);
		optimalPutHere[i].resize(targetLength + 1);
	}

	for (size_t x = 1; x <= sequence.size(); x++)
		minAdditionalLabels[x][0] = 1000*1000; // impossible
	for (size_t y = 1; y <= sequence.size(); y++)
		minAdditionalLabels[0][y] = 0; // impossible
	minAdditionalLabels[0][0] = 0;
	
	for (size_t x = 1; x <= sequence.size(); x++){
		int task = sequence[x-1];
		for (size_t y = 1; y <= targetLength; y++){
			if (x > y) minAdditionalLabels[x][y] = 1000*1000; // impossible
			else {
				int labelNeeded = 1 - sog->labels[begin+y-1].count(task);
				// put label here
				minAdditionalLabels[x][y] = labelNeeded + minAdditionalLabels[x-1][y-1];
				optimalPutHere[x][y] = true;

				//cout << "COMP " << x << " " << y << " " << labelNeeded << " " ;
				//cout << minAdditionalLabels[x][y-1] << " < " << minAdditionalLabels[x][y] << endl;
				if (minAdditionalLabels[x][y-1] < minAdditionalLabels[x][y]){
					minAdditionalLabels[x][y] = minAdditionalLabels[x][y-1];
					optimalPutHere[x][y] = false;
				}
			}
		}
	}

	// get positions by recursion
	int x = sequence.size();
	int y = targetLength;

	while (x != 0 && y != 0){
		if (optimalPutHere[x][y]){
			int task = sequence[x-1];
			sog->labels[begin+y-1].insert(task);
			sog->methodSubTasksToVertices[sequenceID][matchingPositions[x-1]] = begin+y-1;
			x--;
			y--;
		} else {
			y--;
		}
	}
	assert(x == 0);
}


SOG* linearLabelSetOptimisation(Model * htn, vector<pair<int,vector<int>>> & labelSequence){
	// sort by number of labels
	sort(labelSequence.begin(), labelSequence.end(), [&](pair<int,vector<int>> m1, pair<int,vector<int>> m2) {
        return m1.second.size() > m2.second.size();   
    });
	
	// use the SOG as a vehicle to memorise the information
	SOG* sog = new SOG;
	sog->numberOfVertices = labelSequence[0].second.size();	
	sog->labels.resize(sog->numberOfVertices);
	sog->methodSubTasksToVertices.resize(labelSequence.size());

	for (size_t lIndex = 0; lIndex < labelSequence.size(); lIndex++){
		vector<int> & sequence = labelSequence[lIndex].second;
		//cout << "Label " << labelSequence[lIndex].first << " " << sequence.size() << endl;
		vector<int> indices;
		for (size_t i = 0; i < sequence.size(); i++) indices.push_back(i);
		// create the matching
		sog->methodSubTasksToVertices[labelSequence[lIndex].first].resize(sequence.size());
	
		addSequenceToSOG(sog, sequence, indices, labelSequence[lIndex].first, 0, sog->numberOfVertices);
	}

	return sog;
}

void addCompactSequenceToSOG(SOG *sog, vector<int> &sequence, vector<int> &matchingPositions, int sequenceID, Model *htn, unordered_set<int> &effectlessVertices) {
	int lastMacthing = -1;
	for (size_t pos = 0; pos < sequence.size(); pos++) {
		bool foundMatching = false;
		int task = sequence[pos];
		bool isEffectlessAction = task < htn->numActions && (htn->numAdds[task] == 0 && htn->numDels[task] == 0);
		for (size_t v = lastMacthing+1; v < sog->numberOfVertices; v++) {
			if (isEffectlessAction != effectlessVertices.count(v)) continue;
			sog->labels[v].insert(task);
			sog->methodSubTasksToVertices[sequenceID][matchingPositions[pos]] = v;
			foundMatching = true;
			lastMacthing = v;
			break;
		}
		if (!foundMatching) {	
			lastMacthing = sog->numberOfVertices++;
			unordered_set<uint16_t> _temp16;
			unordered_set<uint32_t> _temp32;
			sog->labels.push_back(_temp32);
			sog->adj.push_back(_temp16);
			sog->bdj.push_back(_temp16);
			sog->labels[sog->numberOfVertices - 1].insert(task);
			sog->methodSubTasksToVertices[sequenceID][matchingPositions[pos]] = sog->numberOfVertices - 1;
			if (isEffectlessAction) effectlessVertices.insert(sog->numberOfVertices - 1);
		}
	}
}



SOG* runTOSOGOptimiser(SOG* sog, vector<tuple<uint32_t,uint32_t,uint32_t>> & methods, Model* htn, bool effectLessActionsInSeparateLeafs){
	//cout << endl << endl << endl << "================================================" << endl;
	
	int m = get<0>(methods[0]);
	sog->numberOfVertices = htn->numSubTasks[m];
	sog->labels.resize(sog->numberOfVertices);
	sog->methodSubTasksToVertices.resize(methods.size());

	unordered_set<int> effectlessVertices;
	
	for (size_t mID = 0; mID < methods.size(); mID++){
		int m = get<0>(methods[mID]);
		sog->methodSubTasksToVertices[mID].resize(htn->numSubTasks[m]);
	
		vector<int> taskSequence;
		vector<int> indexSequence;
		for (size_t x = 0; x < htn->numSubTasks[m]; x++){
			int taskID = htn->methodTotalOrder[m][x];
			int task   = htn->subTasks[m][taskID];
			taskSequence.push_back(task);
			indexSequence.push_back(taskID);
		}
		if (!effectLessActionsInSeparateLeafs)
			addSequenceToSOG(sog, taskSequence, indexSequence, mID, 0, sog->numberOfVertices);
		else
			addCompactSequenceToSOG(sog, taskSequence, indexSequence, mID, htn, effectlessVertices);
	}

	sog->adj.resize(sog->numberOfVertices);
	sog->bdj.resize(sog->numberOfVertices);
	for (int i = 0; i < sog->numberOfVertices; i++){
		for (int j = i+1; j < sog->numberOfVertices; j++){
			sog->adj[i].insert(j);
			sog->bdj[j].insert(i);
		}
	}

	return sog;
}	



SOG* runTOSOGOptimiserRecursive(SOG* sog, vector<tuple<uint32_t,uint32_t,uint32_t>> & methods, Model* htn){
	//cout << endl << endl << endl << "================================================" << endl;
	// 1. Step extract the recursive tasks per method
	vector<pair<int,vector<int>>> labelSequence;
	for (size_t mID = 0; mID < methods.size(); mID++){
		int m = get<0>(methods[mID]);
		
		pair<int,vector<int>> seq;
		seq.first = mID;
		for (size_t x = 0; x < htn->numSubTasks[m]; x++)
			if (!htn->sccIsAcyclic[htn->taskToSCC[htn->subTasks[m][htn->methodTotalOrder[m][x]]]])
				seq.second.push_back(htn->subTasks[m][htn->methodTotalOrder[m][x]]);
	
		//cout << "Label" << endl;
		//for (int s : seq.second)
		//	cout << "\t" << htn->taskNames[s] << endl;
		labelSequence.push_back(seq);
	}

	// 2. Step compute an optimal arrangement of the recursive tasks	
	SOG* recSOG = linearLabelSetOptimisation(htn, labelSequence);
	/*for (int i = 0; i < recSOG->numberOfVertices; i++){
		cout << "POS " << i << endl;
		for (const int & l : recSOG->labels[i]){
			 if (!htn->sccIsAcyclic[htn->taskToSCC[l]]) cout << "\t";
			 cout << "\t" << htn->taskNames[l] << endl;
		}
	}*/
	
	// 3. Step determine the length needed for the overall sequence

	// gather positional information for all methods
	vector<vector<pair<int,int>>> spaceNeededInBlocks (methods.size()); 
	for (size_t mID = 0; mID < methods.size(); mID++){
		int m = get<0>(methods[mID]);
		spaceNeededInBlocks[mID].resize(recSOG->numberOfVertices + 1);
		for (size_t i = 0; i < recSOG->numberOfVertices + 1; i++)
			spaceNeededInBlocks[mID][i].second = 0; // initially nothing is needed

		int numRec = 0;
		int lastRecursivePosition = -1;
		int lastRecursive = -1;
		for (size_t x = 0; x < htn->numSubTasks[m]; x++)
			if (!htn->sccIsAcyclic[htn->taskToSCC[htn->subTasks[m][htn->methodTotalOrder[m][x]]]]){
				int positionOfThisTaskInSOG = recSOG->methodSubTasksToVertices[mID][numRec];
				int numberOfTasksSinceLastRecursive = x - lastRecursive - 1;
				
				// memorise how much space we need
				spaceNeededInBlocks[mID][positionOfThisTaskInSOG].first = lastRecursivePosition;
				spaceNeededInBlocks[mID][positionOfThisTaskInSOG].second = numberOfTasksSinceLastRecursive;

				// update last positions
				lastRecursive = x;
				lastRecursivePosition = positionOfThisTaskInSOG;
				numRec++;
			}
		// handle tasks after the last recursive one
		spaceNeededInBlocks[mID][recSOG->numberOfVertices].first = lastRecursivePosition;
		spaceNeededInBlocks[mID][recSOG->numberOfVertices].second =
				htn->numSubTasks[m] - lastRecursive - 1;
	}
	
	
	
	// this is the number of tasks *before* the ith recursive task
	vector<int> blocks(recSOG->numberOfVertices + 1);
	// go over the positions and determine how much space we need up to them
	for (size_t pos = 0; pos < recSOG->numberOfVertices + 1; pos++){
		blocks[pos] = 0; // all blocks initially have size 0
		for (size_t mID = 0; mID < methods.size(); mID++){
			if (spaceNeededInBlocks[mID][pos].second == 0) continue; // this does not need space, so ignore it

			int lastRecursive = spaceNeededInBlocks[mID][pos].first;
			int numTasks = spaceNeededInBlocks[mID][pos].second;

			// check how much space there currently is after the last recursive one
			int currentSpace = 0;
			for (size_t i = lastRecursive + 1; i <= pos; i++)
				currentSpace += blocks[i];

			// if size is not enough increase size of current block
			if (currentSpace < numTasks)
				blocks[pos] += numTasks - currentSpace;
		}
	}
	
	sog->numberOfVertices = 0;
	vector<int> recursivePositions;
	for (size_t pos = 0; pos < recSOG->numberOfVertices + 1; pos++){
		sog->numberOfVertices += blocks[pos];
		recursivePositions.push_back(sog->numberOfVertices);
		sog->numberOfVertices++;
	}
	sog->labels.resize(sog->numberOfVertices);
	sog->methodSubTasksToVertices.resize(methods.size());

	// put the recursive tasks where they belong
	for (size_t mID = 0; mID < methods.size(); mID++){
		int m = get<0>(methods[mID]);
		sog->methodSubTasksToVertices[mID].resize(htn->numSubTasks[m]);
		int numRec = 0;
		
		for (size_t x = 0; x < htn->numSubTasks[m]; x++){
			int taskID = htn->methodTotalOrder[m][x];
			int task = htn->subTasks[m][taskID];
			if (!htn->sccIsAcyclic[htn->taskToSCC[task]]){
				int positionOfThisTaskInSOG = recSOG->methodSubTasksToVertices[mID][numRec];
				// write this task to the SOG
				int posInSOG = recursivePositions[positionOfThisTaskInSOG];
				sog->methodSubTasksToVertices[mID][taskID] = posInSOG;
				sog->labels[posInSOG].insert(htn->subTasks[m][htn->methodTotalOrder[m][x]]);
				numRec++;
			}
		}
	}
	

	for (size_t mID = 0; mID < methods.size(); mID++){
		//cout << "Method " << mID << endl;
		int m = get<0>(methods[mID]);
	
		vector<int> taskSequence;
		vector<int> indexSequence;

		// go through the method and gather the blocks		
		int numRec = 0;
		int lastRecursivePosition = -1;
		for (size_t x = 0; x < htn->numSubTasks[m] + 1; x++){
			bool last = x == htn->numSubTasks[m];
			
			int taskID = (!last)?htn->methodTotalOrder[m][x]:0;
			int task = (!last)?htn->subTasks[m][taskID]:0;
			if (last || !htn->sccIsAcyclic[htn->taskToSCC[task]]){
				//cout << "HANDLE " << numRec << " @ " << x << " of " << htn->numSubTasks[m] << endl;
				int positionOfThisTaskInSOG = (!last)?recSOG->methodSubTasksToVertices[mID][numRec]:
					(recursivePositions.size()-1);
				// determine the range
				int startRange = 0;
				//cout << "\t" << lastRecursivePosition << " " << recursivePositions[lastRecursivePosition] << " " << recursivePositions.size() << endl;
				if (lastRecursivePosition != -1)
					startRange = recursivePositions[lastRecursivePosition] + 1;
				int endRange = recursivePositions[positionOfThisTaskInSOG];
				//cout << "\t" << startRange << " " << endRange << endl;

				// add the partial sequence to the overall sequence
				addSequenceToSOG(sog, taskSequence, indexSequence, mID, startRange, endRange);

				taskSequence.clear();
				indexSequence.clear();
				numRec++;
				lastRecursivePosition = positionOfThisTaskInSOG;
			} else {
				taskSequence.push_back(task);
				indexSequence.push_back(taskID);
			}
		}
	}

	// check the integrity of the SOG ..?


	int numRec = 0;
	for (int i = 0; i < sog->numberOfVertices; i++){
		bool recursive = false;
		for (const unsigned int & l : sog->labels[i])
			recursive |= !htn->sccIsAcyclic[htn->taskToSCC[l]];

		if (recursive) numRec++;
	}

	int methodMaxRec = recSOG->numberOfVertices;
	if (false || numRec != methodMaxRec){
		cout << "SOG: " << numRec << " " << methodMaxRec << " " << methods.size() << endl;

		for (int i = 0; i < sog->numberOfVertices; i++){
			cout << "POS " << i << endl;
			for (const unsigned int & l : sog->labels[i])
				 if (!htn->sccIsAcyclic[htn->taskToSCC[l]])
					 cout << "\t" << htn->taskNames[l] << endl;
		}
		
		for (size_t mID = 0; mID < methods.size(); mID++){
			int m = get<0>(methods[mID]);
			cout << "Method " << mID << " " << htn->methodNames[m] << endl;
			for (size_t x = 0; x < htn->numSubTasks[m]; x++)
				if (!htn->sccIsAcyclic[htn->taskToSCC[htn->subTasks[m][htn->methodTotalOrder[m][x]]]])
					 cout << "\t" << htn->taskNames[htn->subTasks[m][htn->methodTotalOrder[m][x]]] << endl;
		}
	}

	// add edges
	sog->adj.resize(sog->numberOfVertices);
	sog->bdj.resize(sog->numberOfVertices);
	for (int i = 0; i < sog->numberOfVertices; i++){
		for (int j = i+1; j < sog->numberOfVertices; j++){
			sog->adj[i].insert(j);
			sog->bdj[j].insert(i);
		}
	}


	return sog;
}


// just the greedy optimiser
SOG* optimiseSOG(vector<tuple<uint32_t,uint32_t,uint32_t>> & methods, Model* htn, bool effectLessActionsInSeparateLeaf){
	// edge case
	bool allMethodsAreTotallyOrdered = true;
	for (const auto & [m,_1,_2] : methods)
		if (!htn->methodIsTotallyOrdered[m]){
			allMethodsAreTotallyOrdered = false;
			break;
		}

	SOG* sog = new SOG;
	if (methods.size() == 0){
		sog->numberOfVertices = 0;
		return sog;
	}
	
	sort(methods.begin(), methods.end(), [&](tuple<uint32_t,uint32_t,uint32_t> m1, tuple<uint32_t,uint32_t,uint32_t> m2) {
        return htn->numSubTasks[get<0>(m1)] > htn->numSubTasks[get<0>(m2)];   
    });

	int maxSize = 0;
	for (size_t mID = 0; mID < methods.size(); mID++)
		if (maxSize < htn->numSubTasks[get<0>(methods[mID])]) maxSize = htn->numSubTasks[get<0>(methods[mID])];
   	
#ifndef NDEBUG
	if (allMethodsAreTotallyOrdered) cout << "Running TO SOG optimiser";
	else                             cout << "Running PO SOG optimiser";
	cout << " with " << methods.size() << " methods with up to " << maxSize << " subtasks." << endl;
#endif
	if (allMethodsAreTotallyOrdered)
		return runTOSOGOptimiser(sog, methods, htn, effectLessActionsInSeparateLeaf);
		//return runTOSOGOptimiserRecursive(sog, methods, htn);
	else
		return runPOSOGOptimiser(sog, methods, htn, effectLessActionsInSeparateLeaf);
}





void SOG::printDot(Model * htn, ofstream & dfile){
	for (int v = 0; v < numberOfVertices; v++){
		//dfile << "\tv" << v << "[label=\"" << leafOfNode[v] << "\"];" << endl;
		dfile << "\tv" << v << "[label=\"" << v << "\"];" << endl;
	}

	for (int v = 0; v < numberOfVertices; v++){
		for (int n : adj[v])
			dfile << "\tv" << v << " -> v" << n << endl;
	}
}



void SOG::removeTransitiveOrderings(){
	vector<vector<bool>> trans (numberOfVertices);

	for (int x = 0; x < numberOfVertices; x++)
		for (int y = 0; y < numberOfVertices; y++)
			trans[x].push_back(false);
		
	for (int j = 0; j < numberOfVertices; j++)
		for (int nei : adj[j])
			trans[j][nei] = true;
	
	for (int k = 0; k < numberOfVertices; k++)
		for (int x = 0; x < numberOfVertices; x++)
			for (int y = 0; y < numberOfVertices; y++)
				if (adj[x].count(k) && adj[k].count(y)) trans[x][y] = false;

	
	for (int x = 0; x < numberOfVertices; x++){
		adj[x].clear();	
		for (int y = 0; y < numberOfVertices; y++)
			if (trans[x][y])
				adj[x].insert(y);
	}
}


void SOG::succdfs(int n, vector<bool> &visi){
	if (visi[n]) return;
	visi[n] = true;
	successorSet[n].insert(n);

	for (int nei : adj[n]){
		succdfs(nei,visi);
		successorSet[n].insert(successorSet[nei].begin(), successorSet[nei].end());
	}
}

void SOG::calcSucessorSets(){
	successorSet.clear();
	successorSet.resize(numberOfVertices);
	vector<bool> visi (numberOfVertices);

	for (int i = 0; i < numberOfVertices; i++)
		succdfs(i,visi);
}


void SOG::precdfs(int n, vector<bool> &visi){
	if (visi[n]) return;
	visi[n] = true;
	predecessorSet[n].insert(n);

	for (int nei : bdj[n]){
		precdfs(nei,visi);
		predecessorSet[n].insert(predecessorSet[nei].begin(), predecessorSet[nei].end());
	}
}

void SOG::calcPredecessorSets(){
	predecessorSet.clear();
	predecessorSet.resize(numberOfVertices);
	vector<bool> visi (numberOfVertices);

	for (int i = 0; i < numberOfVertices; i++)
		precdfs(i,visi);
}


SOG* generateSOGForLeaf(PDT* leaf){
	SOG * res = new SOG();
	res->numberOfVertices = 1;
	res->adj.resize(1);
	res->bdj.resize(1);
	res->leafOfNode.push_back(leaf);
	res->labels.resize(1);
	for (const int & p : leaf->possiblePrimitives)
		res->labels[0].insert(p);
	for (const int & a : leaf->possibleAbstracts)
		res->labels[0].insert(a);

	return res;
}



SOG* SOG::expandSOG(vector<SOG*> nodeSOGs){
	assert(nodeSOGs.size() == numberOfVertices);
	SOG * res = new SOG();
	vector<vector<int>> mapping(this->numberOfVertices);

	// create new node numbers
	int curnum = 0;
	for (int i = 0; i < nodeSOGs.size(); i++){
		for (int j = 0; j < nodeSOGs[i]->numberOfVertices; j++){
			mapping[i].push_back(curnum++);
			res->labels.push_back(nodeSOGs[i]->labels[j]);
			res->leafOfNode.push_back(nodeSOGs[i]->leafOfNode[j]);
		}
	}


	res->numberOfVertices = curnum;
	res->adj.resize(curnum);
	res->bdj.resize(curnum);

	// Ordering
	for (int i = 0; i < nodeSOGs.size(); i++){
		for (int j = 0; j < nodeSOGs[i]->numberOfVertices; j++){
			for (int nei : nodeSOGs[i]->adj[j]){
				res->adj[mapping[i][j]].insert(mapping[i][nei]);
			}
			for (int nei : nodeSOGs[i]->bdj[j]){
				res->bdj[mapping[i][j]].insert(mapping[i][nei]);
			}
		}
	}


	for (int i = 0; i < numberOfVertices; i++){
		for (int nei : adj[i])
			for (int iMap : mapping[i])
				for (int neiMap : mapping[nei])
					res->adj[iMap].insert(neiMap);
		
		for (int nei : bdj[i])
			for (int iMap : mapping[i])
				for (int neiMap : mapping[nei])
					res->bdj[iMap].insert(neiMap);
	}

	//cout << "Res: " << res->numberOfVertices << endl;
	// TODO potentially expensive ...
	res->removeTransitiveOrderings();
	return res;
}





