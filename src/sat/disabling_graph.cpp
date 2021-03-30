#include <fstream>
#include <queue>

#include "disabling_graph.h"
#include "../Debug.h"
#include "../Invariants.h"

graph::graph(int num){
	numVertices = num;
	adj = new int*[num];
	adjSize = new int[num];
}

graph::~graph(){
	for (size_t i = 0; i < numVertices; i++){
		delete[] adj[i];
	}
	delete[] adj;
	delete[] adjSize;
}

graph::graph(vector<unordered_set<int>> & tempAdj) : graph(tempAdj.size()){
	for (size_t i = 0; i < tempAdj.size(); i++){
		adjSize[i] = tempAdj[i].size();
		adj[i] = new int[adjSize[i]];

		int pos = 0;
		for (const int & x : tempAdj[i])
			adj[i][pos++] = x;
	}
}

int graph::count_edges(){
	int c = 0;
	for (size_t i = 0; i < numVertices; i++)
		c+= adjSize[i];
	return c;
}

// temporal SCC information
int maxdfs; // counter for dfs
bool* U; // set of unvisited nodes
vector<int>* S; // stack
bool* containedS;
int* dfsI;
int* lowlink;

void graph::calcSCCs() {
	cout << "Calculate SCCs..." << endl;

	maxdfs = 0;
	U = new bool[numVertices];
	S = new vector<int>;
	containedS = new bool[numVertices];
	dfsI = new int[numVertices];
	lowlink = new int[numVertices];
	numSCCs = 0;
	vertexToSCC = new int[numVertices];
	for (int i = 0; i < numVertices; i++) {
		U[i] = true;
		containedS[i] = false;
		vertexToSCC[i] = -1;
	}

	for (size_t i = 0; i < numVertices; i++)
		if (U[i]) tarjan(i);

	sccSize = new int[numSCCs];
	for (int i = 0; i < numSCCs; i++)
		sccSize[i] = 0;
	
	for (int i = 0; i < numVertices; i++) {
		int j = vertexToSCC[i];
		sccSize[j]++;
	}
	cout << "- Number of SCCs: " << numSCCs << endl;

	// generate inverse mapping
	sccToVertices = new int*[numSCCs];
	int * currentI = new int[numSCCs];
	for (int i = 0; i < numSCCs; i++)
		currentI[i] = 0;
	for (int i = 0; i < numSCCs; i++) {
		sccToVertices[i] = new int[sccSize[i]];
	}
	for (int i = 0; i < numVertices; i++) {
		int scc = vertexToSCC[i];
		sccToVertices[scc][currentI[scc]] = i;
		currentI[scc]++;
	}

	delete[] currentI;

	delete[] U;
	delete S;
	delete[] containedS;
	delete[] dfsI;
	delete[] lowlink;
}



void graph::tarjan(int v) {
	dfsI[v] = maxdfs;
	lowlink[v] = maxdfs; // v.lowlink <= v.dfs
	maxdfs++;

	S->push_back(v);
	containedS[v] = true;
	U[v] = false; // delete v from U

	for (int i = 0; i < adjSize[v]; i++) { // iterate subtasks -> these are the adjacent nodes
		int v2 = adj[v][i];
		if (U[v2]) {
			tarjan(v2);
			if (lowlink[v] > lowlink[v2]) {
				lowlink[v] = lowlink[v2];
			}
		} else if (containedS[v2]) {
			if (lowlink[v] > dfsI[v2])
				lowlink[v] = dfsI[v2];
		}
	}

	if (lowlink[v] == dfsI[v]) { // root of an SCC
		int v2;
		do {
			v2 = S->back();
			S->pop_back();
			containedS[v2] = false;
			vertexToSCC[v2] = numSCCs;
		} while (v2 != v);
		numSCCs++;
	}
}

void graph::calcSCCGraph() {
	vector<unordered_set<int>> sccg (numSCCs);

	for (int i = 0; i < numVertices; i++) {
		int sccFrom = vertexToSCC[i];
		for (int j = 0; j < adjSize[i]; j++) {
			int sccTo = vertexToSCC[adj[i][j]];
			if (sccFrom != sccTo) {
				sccg[sccFrom].insert(sccTo);
			}
		}
	}
	scc_graph = new graph(sccg);
}

bool graph::can_reach_any_of(vector<int> & from, vector<int> & to){
	unordered_set<int> toSet;
	for (const int & x : to) toSet.insert(x);

	bool * visi = new bool[numVertices];
	for (int i = 0; i < numVertices; i++) visi[i] = false;

	queue<int> q;
	for (const int & x : from) {
		if (toSet.count(x)){
			delete[] visi;
			return true;
		}
		q.push(x);
	}

	while (q.size()){
		int x = q.front();
		q.pop();

		for (int j = 0; j < adjSize[x]; j++){
			int y = adj[x][j];
			if (!visi[y]){
				if (toSet.count(y)) {
					delete[] visi;
					return true;
				}
				visi[y] = true;
				q.push(y);
			}
		}
	}

	delete[] visi;
	return false;
}


bool graph::can_reach_any_of_directly(vector<int> & from, vector<int> & to){
	unordered_set<int> toSet;
	for (const int & x : to) toSet.insert(x);

	for (const int & x : from) {
		if (toSet.count(x)) return true;
		for (int j = 0; j < adjSize[x]; j++){
			int y = adj[x][j];
			if (toSet.count(y)) return true;
		}
	}

	return false;
}


string graph::dot_string(){
	map<int,string> empty;
	return dot_string(empty);
}


string graph::dot_string(map<int,string> names){
	map<int,string> empty;
	return dot_string(names,empty);
}

string graph::dot_string(map<int,string> names, map<int,string> nodestyles){
    string result = "digraph someDirectedGraph{\n";

	// edges
	for (int i = 0; i < numVertices; i++) {
		for (int jI = 0; jI < adjSize[i]; jI++) {
			int j = adj[i][jI];
			result += "\ta" + to_string(i) + " -> a" + to_string(j) + " [label=\"\"];\n";
		}
	}
	
	// vertices
	for (int i = 0; i < numVertices; i++){
		string label = to_string(i);
		if (names.count(i)) label = names[i];

		result += "\ta" + to_string(i) + " [label=\"" + label + "\"";
		if (nodestyles.count(i))
			result += "," + nodestyles[i];
		result += "];\n";
	}

	result += "}\n";

	return result;
}



bool are_actions_applicable_in_the_same_state(Model * htn, int a, int b){
	//return true;
	if (a == b) return true;
	bool counter = false;
    // incompatible preconditions via invariants
    for (int preIdx1 = 0; !counter && preIdx1 < htn->numPrecs[a]; preIdx1++){
		int pre1 = htn->precLists[a][preIdx1];
    	for (int preIdx2 = 0; !counter && preIdx2 < htn->numPrecs[b]; preIdx2++){
			int pre2 = htn->precLists[b][preIdx2];

			counter |= !can_state_features_co_occur(htn, pre1,pre2);
		}
	}

    // checking this is ok for the DG as applying both actions will lead to an inconsistent state
	// 
	// incompatible effects via invariants
    for (int effT1 = 0; !counter && effT1 < 2; effT1++){
		int* effs1 = effT1 ? htn->addLists[a] : htn->delLists[a];
		int effSize1 = effT1 ? htn->numAdds[a] : htn->numDels[a];
		
		for (int effIdx1 = 0; !counter && effIdx1 < effSize1; effIdx1++){
			int eff1 = effs1[effIdx1];
			if (!effT1) eff1 = - eff1 - 1;
    
			// effect of second action
			for (int effT2 = 0; !counter && effT2 < 2; effT2++){
				int* effs2 = effT2 ? htn->addLists[b] : htn->delLists[b];
				int effSize2 = effT2 ? htn->numAdds[b] : htn->numDels[b];
				
				for (int effIdx2 = 0; !counter && effIdx2 < effSize2; effIdx2++){
					int eff2 = effs2[effIdx2];
					if (!effT2) eff2 = - eff2 - 1;
			
					counter |= !can_state_features_co_occur(htn, eff1, eff2);
				}
			}
		}
	}

	return !counter;
}

graph * compute_disabling_graph(Model * htn, bool no_invariant_inference){
	cout << endl << "Computing Disabling Graph" << endl;
	std::clock_t dg_start = std::clock();

	vector<unordered_set<int>> tempAdj (htn->numActions);
	for (int f = 0; f < htn->numStateBits; f++){
		for (int deletingIndex = 0; deletingIndex < htn->delToActionSize[f]; deletingIndex++){
			int deletingAction = htn->delToAction[f][deletingIndex];
			
			for (int needingIndex = 0; needingIndex < htn->precToActionSize[f]; needingIndex++){
				int needingAction = htn->precToAction[f][needingIndex];
				if (deletingAction == needingAction) continue; // action cannot disable itself
				if (!no_invariant_inference &&
						!are_actions_applicable_in_the_same_state(htn, deletingAction, needingAction))
					continue;
				DEBUG(
					cout << "DEL NEED: " << deletingAction << " " << htn->taskNames[deletingAction];
					cout << " vs " << needingAction << " " << htn->taskNames[needingAction] << endl;
					);

				tempAdj[deletingAction].insert(needingAction);
			}
		}

		if (no_invariant_inference){
			for (int addingIndex = 0; addingIndex < htn->addToActionSize[f]; addingIndex++){
				int addingAction = htn->addToAction[f][addingIndex];
				
				for (int needingIndex = 0; needingIndex < htn->precToActionSize[f]; needingIndex++){
					int needingAction = htn->precToAction[f][needingIndex];
					if (addingAction == needingAction) continue; // action cannot disable itself
					if (!no_invariant_inference &&
							!are_actions_applicable_in_the_same_state(htn, addingAction, needingAction))
						continue;
					DEBUG(
						cout << "NEED ADD: " << needingAction << " " << htn->taskNames[needingAction];
						cout << " vs " << addingAction << " " << htn->taskNames[addingAction] << endl;
						);

					tempAdj[addingAction].insert(needingAction);
				}
			}

			for (int addingIndex = 0; addingIndex < htn->addToActionSize[f]; addingIndex++){
				int addingAction = htn->addToAction[f][addingIndex];
				
				for (int deletingIndex = 0; deletingIndex < htn->delToActionSize[f]; deletingIndex++){
					int deletingAction = htn->delToAction[f][deletingIndex];
					if (addingAction == deletingAction) continue; // action cannot disable itself
					if (!no_invariant_inference &&
							!are_actions_applicable_in_the_same_state(htn, addingAction, deletingAction))
						continue;
					DEBUG(
						cout << "DEL-ADD " << deletingAction << " " << htn->taskNames[deletingAction];
						cout << " vs " << addingAction << " " << htn->taskNames[addingAction] << endl;
						);

					tempAdj[addingAction].insert(deletingAction);
					tempAdj[deletingAction].insert(addingAction);
				}
			}
		}
	}
	
	// convert into int data structures
	graph * dg = new graph(tempAdj);
	cout << "Generated graph with " << dg->count_edges() << " edges." << endl;
	dg->calcSCCs();
	map<int,int> scc_sizes;
	for (int scc = 0; scc < dg->numSCCs; scc++)
		scc_sizes[dg->sccSize[scc]]++;

	cout << "SCC sizes:";
	for (auto [size,num] : scc_sizes)
		cout << " " << num << "x" << size;
	cout << endl;

	std::clock_t dg_end = std::clock();
	double dg_time = 1000.0 * (dg_end-dg_start) / CLOCKS_PER_SEC;
	cout << "Generating the graph took " << dg_time << "ms." << endl;

	return dg;
}

bool doActionsInterfere(Model * htn, vector<int> & previous, vector<int> & next){
	//set<int> prevSet;
	//for (int p : previous)
	//	prevSet.insert(p);

	// don't need to handle identical actions as it is ok to repeat them ...
	//for (int n : next)
	//	if (prevSet.count(n)) return true; // two identical actions may not be executed at the same time.
	
	unordered_set<int> nextPreconditions;
	unordered_set<int> nextAdds;
	unordered_set<int> nextDeletes;
	for (const int & n : next){
		for (size_t p = 0; p < htn->numPrecs[n]; p++)
			nextPreconditions.insert(htn->precLists[n][p]);
		for (size_t a = 0; a < htn->numAdds[n]; a++)
			nextAdds.insert(htn->addLists[n][a]);
		for (size_t d = 0; d < htn->numDels[n]; d++)
			nextDeletes.insert(htn->delLists[n][d]);
	}

	// check if anyone adds or deletes
	for (const int & p : previous){
		for (size_t a = 0; a < htn->numAdds[p]; a++)
			if (nextPreconditions.count(htn->addLists[p][a]) || 
					nextDeletes.count(htn->addLists[p][a]))
				return true;
		for (size_t d = 0; d < htn->numDels[p]; d++)
			if (nextPreconditions.count(htn->delLists[p][d]) || 
					nextAdds.count(htn->delLists[p][d])) return true;
	}
	return false;
}


vector<vector<int>> compute_block_compression(Model * htn, vector<PDT*> & leafs){
	vector<vector<int>> blocks;

	vector<int> currentBlock;
	vector<int> currentPrimitives;

	for (size_t l = 0; l < leafs.size(); l++){
		// try to extend
		vector<int> nonPrunedPrimitives;
		for (size_t pI = 0; pI < leafs[l]->possiblePrimitives.size(); pI++){
			if (!leafs[l]->prunedPrimitives[pI])
				nonPrunedPrimitives.push_back(leafs[l]->possiblePrimitives[pI]);
		}
#ifndef NDEBUG
		/*cout << "Current Primitives: " << endl;
		for (int cur : currentPrimitives)
			cout << htn->taskNames[cur] << endl;
		
		cout << "Next Primitives: " << endl;
		for (int npp : nonPrunedPrimitives)
			cout << htn->taskNames[npp] << endl;*/
#endif

		if (doActionsInterfere(htn, currentPrimitives,nonPrunedPrimitives)){
#ifndef NDEBUG
			//cout << "Interference." << endl << endl << endl;
#endif
			// one of these actions will disable another
			blocks.push_back(currentBlock); // new block
			currentBlock.clear();
			currentPrimitives.clear();
		}

		currentBlock.push_back(l);
		for (const int & x : nonPrunedPrimitives)
			currentPrimitives.push_back(x);
	}

	blocks.push_back(currentBlock);

	return blocks;
}

//vector<vector<int>> compute_block_compression(Model * htn, graph * dg, vector<PDT*> & leafs){
//	vector<vector<int>> blocks;
//
//	vector<int> currentBlock;
//	vector<int> currentPrimitives;
//
//	for (size_t l = 0; l < leafs.size(); l++){
//		// try to extend
//		bool allNoDel = true;
//		for (const int & a : currentPrimitives)
//			allNoDel &= htn->numDels[a] == 0;
//		
//		if (!allNoDel){
//			// one of these actions will disable another
//			blocks.push_back(currentBlock); // new block
//			currentBlock.clear();
//			currentPrimitives.clear();
//		}
//
//		currentBlock.push_back(l);
//		for (const int & x : leafs[l]->possiblePrimitives)
//			currentPrimitives.push_back(x);
//	}
//
//	blocks.push_back(currentBlock);
//
//	return blocks;
//}







