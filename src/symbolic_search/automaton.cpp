#include "automaton.h"

#include <vector>
#include <string>
#include <map>
#include <cassert>
#include <iostream>
#include <queue>
#include <fstream>
#include <unordered_set>


std::vector<string> vertex_names;
std::vector<int> vertex_to_method;
// adjacency matrix, map contains:
// 1. task on the edge
// 2. the target vertex
// 3. the BDD
std::vector<std::map<int,std::map<int, void* >>> edges; 


std::string to_string(Model * htn){
	std::string s = "digraph graphname {\n";

	for (int v = 0; v < vertex_names.size(); v++)
		//s += "  v" + std::to_string(v) + "[label=\"" + vertex_names[v] + "\"];\n";
		s += "  v" + std::to_string(v) + "[label=\"" + std::to_string(vertex_to_method[v]) + "\"];\n";
	
	s+= "\n\n";

	for (int v = 0; v < vertex_names.size(); v++)
		for (auto & [task,tos] : edges[v])
			for (auto & [to,bdd] : tos)
				s += "  v" + std::to_string(v) + " -> v" + std::to_string(to) + " [label=\"" + 
					//htn->taskNames[task] + "\"]\n";
					std::to_string(task) + "\"]\n";

	return s + "}";
}


void graph_to_file(Model* htn, std::string file){
	std::ofstream out(file);
    out << to_string(htn);
    out.close();
}

void build_automaton(Model * htn){
	std::vector<int> methods_with_two_tasks;
	// the vertex number of these methods, 0 and 1 are start and end vertex
	std::map<int,int> methods_with_two_tasks_vertex;
	vertex_to_method.push_back(-1);
	vertex_to_method.push_back(-2);
	for (int m = 0; m < htn->numMethods; m++)
		if (htn->numSubTasks[m] == 2){
			methods_with_two_tasks.push_back(m);
			methods_with_two_tasks_vertex[m] = methods_with_two_tasks.size() + 1;
			vertex_to_method.push_back(m);
		}


	int number_of_vertices = 2 + methods_with_two_tasks.size();
	edges.resize(number_of_vertices);

	// build the initial version of the graph
	vertex_names.push_back("start");
	vertex_names.push_back("end");
	std::map<int,std::pair<int,int>> tasks_per_method; // first and second
	for (auto & [method, vertex] : methods_with_two_tasks_vertex){
		vertex_names.push_back(htn->methodNames[method]);

		// which one is the second task of this method
		int first = htn->subTasks[method][0];
		int second = htn->subTasks[method][1];
		assert(htn->numOrderings[method] == 1);
		if (htn->ordering[method][0] == 1 && htn->ordering[method][1])
			std::swap(first,second);

		edges[0][first][vertex] = nullptr;
		tasks_per_method[method] = {first, second};
	}


	// add the initial abstract task
	edges[0][htn->initialTask][1] = nullptr;



	// apply transition rules
	
	// loop over the outgoing edges of 0, only to those rules can be applied

	std::queue<std::tuple<int,int,int>> q;
	std::vector<std::unordered_set<int>> eps(number_of_vertices);
	for (auto & [task,tos] : edges[0])
		for (auto & [to,bdd] : tos)
			q.push({0,task,to});

	while (q.size()){
		int from = get<0>(q.front());
		int task = get<1>(q.front());
		int to   = get<2>(q.front());
		q.pop();

		if (from != 0) continue;
			
		if (task < htn->numActions){
			// primitive edge
			if (!eps[to].count(0)){
					eps[to].insert(0);

					for (auto & [task2,tos] : edges[to])
						for (auto & [to2,bdd] : tos){
							edges[0][task2][to2] = nullptr;
							q.push({0, task2, to2});
						}

					if (to == 1){
						std::cout << "Goal test" << std::endl;
					}
				}
		} else {
			// abstract edge, go over all applicable methods	
		
			for(int mIndex = 0; mIndex < htn->numMethodsForTask[task]; mIndex++){
				int method = htn->taskToMethods[task][mIndex];

				// cases
				if (htn->numSubTasks[method] == 0){
					if (!eps[to].count(0)){
						eps[to].insert(0);

						for (auto & [task2,tos] : edges[to])
							for (auto & [to2,bdd] : tos){
								edges[0][task2][to2] = nullptr;
								q.push({0, task2, to2});
							}

						if (to == 1){
							std::cout << "Goal test" << std::endl;
						}
					}
				} else if (htn->numSubTasks[method] == 1){
					edges[0][htn->subTasks[method][0]][to] = nullptr;
					q.push({0, htn->subTasks[method][0], to});
				} else { // two subtasks
					assert(htn->numSubTasks[method] == 2);
					// add edge
					edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to] = nullptr;
					for (int p2 : eps[methods_with_two_tasks_vertex[method]]){
						assert(p2 == 0);
						edges[p2][tasks_per_method[method].second][to] = nullptr;
						q.push({p2,tasks_per_method[method].second, to});
					}
				}
			}
		}
	}








	//std::cout << to_string(htn) << std::endl;
	graph_to_file(htn,"graph.dot");
}

