#include "automaton.h"
#include "sym_variables.h"
#include "transition_relation.h"
#include "cuddObj.hh"

#include <chrono>
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
// 3. the cost
// 4. the abstract task round + 1   -    or 0 for the primitive round
// 4. the BDD
std::vector<std::map<int,std::map<int, std::map<int, std::map<int, BDD>>>>> edges; 


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

void ensureBDD(int from, int task, int to, int cost, int abstractRound, symbolic::SymVariables & sym_vars){
	if (!edges[from].count(task) || !edges[from][task].count(to) || !edges[from][task][to].count(cost) || !edges[from][task][to][cost].count(abstractRound))
		edges[from][task][to][cost][abstractRound] = sym_vars.zeroBDD();

}
void ensureBDD(int task, int to, int cost, int abstractRound, symbolic::SymVariables & sym_vars){
	ensureBDD(0,task,to,cost, abstractRound, sym_vars);
}

void build_automaton(Model * htn){

	// Smyoblic Playground
	symbolic::SymVariables sym_vars(htn);
	sym_vars.init(true);
	BDD init = sym_vars.getStateBDD(htn->s0List, htn->s0Size);
	BDD goal = sym_vars.getPartialStateBDD(htn->gList, htn->gSize);
	
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

		//edges[0][first][vertex] = sym_vars.oneBDD();
		//edges[0][first][vertex] = sym_vars.zeroBDD();
		//for (int i = 0; i < htn->numVars; i++)
		//	edges[0][first][vertex] *= sym_vars.auxBiimp(i); // v_i = v_i''
	  	//sym_vars.bdd_to_dot(edges[0][first][vertex], "m_biimp" + std::to_string(method) + ".dot");
		tasks_per_method[method] = {first, second};
	}


	// add the initial abstract task
	edges[0][htn->initialTask][1][0][0] = init;



	// apply transition rules
	
	// loop over the outgoing edges of 0, only to those rules can be applied

	std::map<int,std::deque<std::tuple<int,int>>> prim_q;
	std::map<int,std::map<int,std::deque<std::tuple<int,int>>>> abst_q;
	std::vector<std::map<int,std::map<int,BDD>>> eps;

	std::map<int,std::map<int,BDD>> _empty_eps;
	_empty_eps[0][0] = sym_vars.zeroBDD();
	for (int i = 0; i < number_of_vertices; i++) eps.push_back(_empty_eps);

#define put push_back
//#define put insert

	abst_q[0][1].put({htn->initialTask,1});

	// cost of current layer and whether we are in abstract or primitive mode
	int currentCost = 0;
	int currentDepthInAbstract = 0; // will be zero for the primitive round
	
	auto addQ = [&] (int task, int to, int extraCost) {
		if (task >= htn->numActions || htn->actionCosts[task] == 0){
			// abstract task or zero-cost action
			abst_q[currentCost + extraCost][currentDepthInAbstract+1].put({task,to});
		} else {
			// primitive with cost
			prim_q[currentCost + extraCost + htn->actionCosts[task]].put({task,to});
		}
	};
	
	std::clock_t layer_start = std::clock();
	int lastCost;
	int lastDepth;

	int step = 0;
	std::deque<std::tuple<int,int>> & current_queue = prim_q[0];
	while (true){ // TODO: detect unsolvability
		
		// TODO separate trans and rel
		if (current_queue.size() == 0){
			std::clock_t layer_end = std::clock();
			double layer_time_in_ms = 1000.0 * (layer_end-layer_start) / CLOCKS_PER_SEC;
			std::cout << "Layer time: " << layer_time_in_ms << "ms" << std::endl << std::endl;
			layer_start = layer_end;

			lastCost = currentCost;
			lastDepth = currentDepthInAbstract;

			// check whether we have to stay abstract
			std::map<int,std::deque<std::tuple<int,int>>>::iterator next_abstract_layer;
			if (currentDepthInAbstract == 0){
				next_abstract_layer = abst_q[currentCost].begin();
			} else {
				next_abstract_layer = abst_q[currentCost].find(currentDepthInAbstract);
				if (next_abstract_layer != abst_q[currentCost].end()){
					next_abstract_layer++;
				}
			}
			
			if (next_abstract_layer == abst_q[currentCost].end()){
				// transition to primitive layer
				currentCost = (++prim_q.find(currentCost))->first;
				currentDepthInAbstract = 0; // the primitive layer
				current_queue = prim_q[currentCost];
				std::cout << "========================== Cost Layer " << currentCost << std::endl; 
			} else {
				currentDepthInAbstract = next_abstract_layer->first;
				current_queue = next_abstract_layer->second;
				std::cout << "========================== Abstract layer of cost " << currentCost << " layer: " << currentDepthInAbstract << std::endl; 
			}

			std::cout << "COPY " << std::endl; 

			// copy the graph for the new cost layer
			for (int from = 0; from < edges.size(); from++)
				for (auto & [task,es] : edges[from])
					for (auto & [to, bdds] : es){
						if (!bdds.count(lastCost)) continue;
						if (!bdds[lastCost].count(lastDepth)) continue;
						ensureBDD(from,task,to,currentCost,currentDepthInAbstract,sym_vars); // might already contain a BDD from epsilon transitions
						edges[from][task][to][currentCost][currentDepthInAbstract] += 
							bdds[lastCost][lastDepth];
					}
			
			std::cout << "edges done" << std::endl; 
			
			// epsilon set
			for (int from = 0; from < edges.size(); from++)
				eps[from][currentCost][currentDepthInAbstract] = eps[from][lastCost][lastDepth];
			
			std::cout << "eps done" << std::endl; 
			
			continue;
		}
		
		int task = get<0>(current_queue.front());
		int to   = get<1>(current_queue.front());
		current_queue.pop_front();
		//int task = get<0>(*cq.begin());
		//int to   = get<1>(*cq.begin());
		//cq.erase(cq.begin());

		//ensureBDD(task, to, sym_vars); // necessary?
		BDD state = edges[0][task][to][lastCost][lastDepth];
		//sym_vars.bdd_to_dot(state, "state" + std::to_string(step) + ".dot");
	  	
		if (state == sym_vars.zeroBDD()) continue; // impossible state, don't treat it

		if (step % 1== 0){
			std::cout << "STEP #" << step << ": " << task << " " << vertex_to_method[to] << std::endl;
	   		std::cout << "\t\t" << htn->taskNames[task] << std::endl;		
		}

		if (task < htn->numActions){
			std::cout << "Prim: " << htn->taskNames[task] << std::endl;
			// apply action to state
			BDD nextState = trs[task].image(state);
			nextState = nextState.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);
			//sym_vars.bdd_to_dot(nextState, "nextState" + std::to_string(step) + ".dot");
			//sym_vars.bdd_to_dot(eps[to], "eps" + std::to_string(step) + ".dot");

			// check if already added
			// TODO: there is a bit faster code here
 			BDD disjunct = eps[to][currentCost][currentDepthInAbstract] + nextState;  
			if (disjunct != eps[to][currentCost][currentDepthInAbstract]){
				eps[to][currentCost][currentDepthInAbstract] = disjunct; // TODO: check logic here

				std::cout << "LOOP" << std::endl;
			
				for (auto & [task2,tos] : edges[to])
					for (auto & [to2,bdds] : tos){
						if (!bdds.count(lastCost) || !bdds[lastCost].count(lastDepth)) continue;
						BDD bdd = bdds[lastCost][lastDepth];
						BDD addState = nextState.AndAbstract(bdd,sym_vars.existsVarsEff);

						
						ensureBDD(task2, to2, currentCost, currentDepthInAbstract, sym_vars);
						
						BDD edgeDisjunct = edges[0][task2][to2][currentCost][currentDepthInAbstract] + addState;
						if (edgeDisjunct != edges[0][task2][to2][currentCost][currentDepthInAbstract]){
					   		edges[0][task2][to2][currentCost][currentDepthInAbstract] = edgeDisjunct;
							//std::cout << "\tPrim: " << task2 << " " << vertex_to_method[to2] << std::endl;
							addQ(task2, to2, 0);
						} else {
							//std::cout << "\tKnown state: " << task2 << " " << vertex_to_method[to2] << std::endl;
						}
					}

				if (to == 1 && nextState * goal != sym_vars.zeroBDD()){
					std::cout << "Goal reached! Length=" << currentCost << " steps=" << step << std::endl;
	  				// sym_vars.bdd_to_dot(nextState, "goal.dot");
					exit(0);
					return;
				}
			} else {
				//std::cout << "\tNot new for Eps" << std::endl;
			}
		} else {
			// abstract edge, go over all applicable methods	
		
			for(int mIndex = 0; mIndex < htn->numMethodsForTask[task]; mIndex++){
				int method = htn->taskToMethods[task][mIndex];
				//std::cout << "\t==Method " << method << std::endl;

				// cases
				if (htn->numSubTasks[method] == 0){

					BDD nextState = state;
					nextState = nextState.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);

					// check if already added
					BDD disjunct = eps[to][currentCost][currentDepthInAbstract] + nextState;
					if (disjunct != eps[to][currentCost][currentDepthInAbstract]){
						eps[to][currentCost][currentDepthInAbstract] = disjunct;
	
						for (auto & [task2,tos] : edges[to])
							for (auto & [to2,bdds] : tos){
								if (!bdds.count(lastCost) || !bdds[lastCost].count(lastDepth)) continue;
								BDD bdd = bdds[lastCost][lastDepth];
								
								BDD addState = nextState.AndAbstract(bdd,sym_vars.existsVarsEff);
								
								ensureBDD(task2,to2,currentCost,currentDepthInAbstract,sym_vars);
								BDD edgeDisjunct = edges[0][task2][to2][currentCost][currentDepthInAbstract] + addState;
								if (edgeDisjunct != edges[0][task2][to2][currentCost][currentDepthInAbstract]){
							   		edges[0][task2][to2][currentCost][currentDepthInAbstract] = edgeDisjunct;
									//std::cout << "\tEmpty Method: " << task2 << " " << vertex_to_method[to2] << std::endl;
									addQ(task2, to2, 0);
								}
							}
	
						if (to == 1 && nextState * goal != sym_vars.zeroBDD()){
							std::cout << "Goal reached! Length=" << currentCost << " steps=" << step <<  std::endl;
							exit(0);
							return;
						}
					}
				} else if (htn->numSubTasks[method] == 1){
					// ensure that a BDD is there
					ensureBDD(htn->subTasks[method][0], to, currentCost, currentDepthInAbstract, sym_vars);
					
					// perform the operation
					BDD disjunct = edges[0][htn->subTasks[method][0]][to][currentCost][currentDepthInAbstract] + state;
					if (disjunct != edges[0][htn->subTasks[method][0]][to][currentCost][currentDepthInAbstract]){
						edges[0][htn->subTasks[method][0]][to][currentCost][currentDepthInAbstract] = disjunct;
						//std::cout << "\tUnit: " << htn->subTasks[method][0] << " " << vertex_to_method[to] << std::endl;
						addQ(htn->subTasks[method][0], to, 0);
					}
					
				} else { // two subtasks
					assert(htn->numSubTasks[method] == 2);
					// add edge (state is irrelevant here!!)
				
					BDD r_temp = state.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff);
				
					ensureBDD(methods_with_two_tasks_vertex[method], tasks_per_method[method].second, to, currentCost, currentDepthInAbstract, sym_vars);
				
					BDD disjunct_r_temp = edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract] + r_temp;
					if (disjunct_r_temp != edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract]){
						//addQ(tasks_per_method[method].first, methods_with_two_tasks_vertex[method]);
						edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract] = disjunct_r_temp;
					}
					
					
					for (int cost = 0; cost <= lastCost; cost++){
						BDD stateAtRoot = eps[methods_with_two_tasks_vertex[method]][cost][0];
						BDD newState = stateAtRoot.AndAbstract(state,sym_vars.existsVarsEff);
													
						if (!newState.IsZero()){
							int targetCost = currentCost + cost;
							ensureBDD(tasks_per_method[method].second,to,targetCost,0, sym_vars); // add as an edge to the future
							BDD disjunct = edges[0][tasks_per_method[method].second][to][targetCost][0] + newState;
							if (edges[0][tasks_per_method[method].second][to][targetCost][0] != disjunct){
								edges[0][tasks_per_method[method].second][to][targetCost][0] = disjunct;
								//std::cout << "\t2 EPS: " << tasks_per_method[method].second << " " << vertex_to_method[to] << std::endl;
								addQ(tasks_per_method[method].second, to, targetCost); // TODO think about when to add ... the depth in abstract should be irrelevant?
							}
						}
					}

					
					// new state for edge to method vertex
					ensureBDD(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], currentCost, currentDepthInAbstract, sym_vars);
		
					BDD biimp = sym_vars.oneBDD();
					for (int i = 0; i < htn->numVars; i++)
						biimp *= sym_vars.auxBiimp(i); // v_i = v_i''

					BDD ss = state.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
					ss *= biimp;
		
					BDD disjunct2 = edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][currentCost][currentDepthInAbstract] + ss;
				   	if (disjunct2 != edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][currentCost][currentDepthInAbstract]){
					   edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][currentCost][currentDepthInAbstract] = disjunct2;
						
					   //std::cout << "\t2 normal: " << tasks_per_method[method].first << " " << vertex_to_method[methods_with_two_tasks_vertex[method]] << std::endl;
					   addQ(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], 0);
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

