#include "automaton.h"
#include "sym_variables.h"
#include "transition_relation.h"
#include "cuddObj.hh"

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
std::vector<std::map<int,std::map<int, BDD>>> edges; 


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

void ensureBDD(int task, int to, symbolic::SymVariables & sym_vars){
	if (!edges[0].count(task) || !edges[0][task].count(to))
		edges[0][task][to] = sym_vars.zeroBDD();

}


void build_automaton(Model * htn){

	// Smyoblic Playground
	symbolic::SymVariables sym_vars(htn);
	sym_vars.init();
	BDD init = sym_vars.getStateBDD(htn->s0List, htn->s0Size);
	sym_vars.bdd_to_dot(init, "init.dot");
	std::vector<symbolic::TransitionRelation> trs;
	for (int i = 0; i < htn->numActions; ++i) {
	  std::cout << "Creating TR " << i << std::endl;
	  trs.emplace_back(&sym_vars, i, htn->actionCosts[i]);
	  trs.back().init(htn);
	  //sym_vars.bdd_to_dot(trs.back().getBDD(), "op" + std::to_string(i) + ".dot");
	}



	// actual automaton construction
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
		assert(htn->numOrderings[method] == 2);
		if (htn->ordering[method][0] == 1 && htn->ordering[method][1])
			std::swap(first,second);

		//edges[0][first][vertex] = sym_vars.zeroBDD();
		tasks_per_method[method] = {first, second};
	}


	// add the initial abstract task
	edges[0][htn->initialTask][1] = init;



	// apply transition rules
	
	// loop over the outgoing edges of 0, only to those rules can be applied

	std::queue<std::tuple<int,int>> q;
	std::vector<BDD> eps;
	for (int i = 0; i < number_of_vertices; i++)
		eps.push_back(sym_vars.zeroBDD());

	for (auto & [task,tos] : edges[0])
		for (auto & [to,bdd] : tos)
			q.push({task,to});

	int step = 0;
	while (q.size()){
		int task = get<0>(q.front());
		int to   = get<1>(q.front());
		q.pop();

		ensureBDD(task, to, sym_vars); // necessary?
		BDD state = edges[0][task][to];
	  	//sym_vars.bdd_to_dot(state, "state" + std::to_string(step) + ".dot");
		
		if (state == sym_vars.zeroBDD()) continue; // impossible state, don't treat it

		std::cout << "STEP #" << step << ": " << task << " " << vertex_to_method[to] << std::endl; 

		if (task < htn->numActions){
			// apply action to state
			BDD nextState = trs[task].image(state);
	  		//sym_vars.bdd_to_dot(nextState, "nextstate" + std::to_string(step) + ".dot");

			// check if already added
			BDD disjunct = eps[to] + nextState;
			if (disjunct != eps[to]){
				eps[to] = disjunct;

				for (auto & [task2,tos] : edges[to])
					for (auto & [to2,bdd] : tos){
						ensureBDD(task2,to2,sym_vars);
						BDD edgeDisjunct = edges[0][task2][to2] + nextState;
						if (edgeDisjunct != edges[0][task2][to2]){
					   		edges[0][task2][to2] = edgeDisjunct;
							std::cout << "\tPrim: " << task2 << " " << vertex_to_method[to2] << std::endl;
							q.push({task2, to2});
						}
					}

				if (to == 1){
					std::cout << "Goal reached!" << std::endl;
	  				sym_vars.bdd_to_dot(nextState, "goal.dot");
					exit(0);
					return;
				}
			}
		} else {
			// abstract edge, go over all applicable methods	
		
			for(int mIndex = 0; mIndex < htn->numMethodsForTask[task]; mIndex++){
				int method = htn->taskToMethods[task][mIndex];
				std::cout << "\t==Method " << method << std::endl;

				// cases
				if (htn->numSubTasks[method] == 0){

					// check if already added
					BDD disjunct = eps[to] + state;
					if (disjunct != eps[to]){
						eps[to] = disjunct;
	
						for (auto & [task2,tos] : edges[to])
							for (auto & [to2,bdd] : tos){
								ensureBDD(task2,to2,sym_vars);
								BDD edgeDisjunct = edges[0][task2][to2] + state;
								if (edgeDisjunct != edges[0][task2][to2]){
							   		edges[0][task2][to2] = edgeDisjunct;
									std::cout << "\tEmpty Method: " << task2 << " " << vertex_to_method[to2] << std::endl;
									q.push({task2, to2});
								}
							}
	
						if (to == 1){
							std::cout << "Goal reached!" << std::endl;
							exit(0);
							return;
						}
					}
				} else if (htn->numSubTasks[method] == 1){
					// ensure that a BDD is there
					ensureBDD(htn->subTasks[method][0], to, sym_vars);
					
					// perform the operation
					BDD disjunct = edges[0][htn->subTasks[method][0]][to] + state;
					if (disjunct != edges[0][htn->subTasks[method][0]][to]){
						edges[0][htn->subTasks[method][0]][to] = disjunct;
						std::cout << "\tUnit: " << htn->subTasks[method][0] << " " << vertex_to_method[to] << std::endl;
						q.push({htn->subTasks[method][0], to});
					}
					
				} else { // two subtasks
					assert(htn->numSubTasks[method] == 2);
					// add edge (state is irrelevant here!!)
					edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to] = sym_vars.zeroBDD();
				
					BDD stateAtRoot = eps[methods_with_two_tasks_vertex[method]];
					BDD conjuct = stateAtRoot * state;
	  									
					if (conjuct != sym_vars.zeroBDD()){
						ensureBDD(tasks_per_method[method].second, to, sym_vars);
						BDD disjunct = edges[0][tasks_per_method[method].second][to] + conjuct;
						if (edges[0][tasks_per_method[method].second][to] != disjunct){
							edges[0][tasks_per_method[method].second][to] = disjunct;
							std::cout << "\t2 EPS: " << tasks_per_method[method].second << " " << vertex_to_method[to] << std::endl;
							q.push({tasks_per_method[method].second, to});
						}
					}

					// new state for edge to method vertex
					ensureBDD(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], sym_vars);
					BDD disjunct = edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]] + state;
				   if (disjunct != edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]]){
					   edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]] = disjunct;
						
					   std::cout << "\t2 normal: " << tasks_per_method[method].first << " " << vertex_to_method[methods_with_two_tasks_vertex[method]] << std::endl;
					   q.push({tasks_per_method[method].first, methods_with_two_tasks_vertex[method]});
				   }	
				}
			}
		}

		//graph_to_file(htn,"graph" + std::to_string(step++) + ".dot");
		step++;
	}

	std::cout << "Ending ..." << std::endl;
	exit(0);
	//delete sym_vars.manager.release();






	//std::cout << to_string(htn) << std::endl;
}

