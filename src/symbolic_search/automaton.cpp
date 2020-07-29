#include "../debug.h"
#include "automaton.h"
#include "sym_variables.h"
#include "transition_relation.h"
#include "cuddObj.hh"

#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <stack>
#include <cassert>
#include <iostream>
#include <iomanip>
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
// 5. the BDD
std::vector<std::map<int,std::map<int, std::map<int, std::map<int, BDD>>>>> edges; 

// 5. with which state do we incur the extra cost of 3.
std::vector<std::map<int,std::map<int, std::map<int, BDD>>>> edgesExtraCost;
std::vector<std::map<int,std::map<int, std::map<int, std::vector<std::pair<int,int>>>>>> edgesExtraCostSource;

std::map<int,std::pair<int,int>> tasks_per_method; // first and second
	
std::vector<std::map<int,std::map<int,BDD>>> eps;

// XXX hacky
std::map<int,int> methods_with_two_tasks_vertex;

std::string to_string(Model * htn){
	std::string s = "digraph graphname {\n";

	for (int v = 0; v < vertex_names.size(); v++)
		//s += "  v" + std::to_string(v) + "[label=\"" + vertex_names[v] + "\"];\n";
		s += "  v" + std::to_string(v) + "[label=\"" + std::to_string(vertex_to_method[v]) + "\"];\n";
	
	s+= "\n\n";


	for (int v = 0; v < vertex_names.size(); v++)
		for (auto & [task,tos] : edges[v])
			for (auto & [to,bdd] : tos)
				s += "  v" + std::to_string(v) + " -> v" + std::to_string(to) + " [label=\"" + std::to_string(task) + "\"]\n";
	
	return s + "}";
}


void graph_to_file(Model* htn, std::string file){
	std::ofstream out(file);
    out << to_string(htn);
    out.close();
}

void ensureBDDExtraCost(int from, int task, int to, int cost, symbolic::SymVariables & sym_vars){
	if (!edgesExtraCost[from].count(task) || !edgesExtraCost[from][task].count(to) || !edgesExtraCost[from][task][to].count(cost))
		edgesExtraCost[from][task][to][cost] = sym_vars.zeroBDD();

}
void ensureBDD(int from, int task, int to, int cost, int abstractRound, symbolic::SymVariables & sym_vars){
	if (!edges[from].count(task) || !edges[from][task].count(to) || !edges[from][task][to].count(cost) || !edges[from][task][to][cost].count(abstractRound))
		edges[from][task][to][cost][abstractRound] = sym_vars.zeroBDD();

}
void ensureBDD(int task, int to, int cost, int abstractRound, symbolic::SymVariables & sym_vars){
	ensureBDD(0,task,to,cost, abstractRound, sym_vars);
}





std::vector<symbolic::TransitionRelation> trs;


//================================================== extract solution


typedef std::tuple<int,int,int,int,int,int,int,int> tracingInfo;

void printStack(std::deque<int> & taskStack, std::deque<int> & methodStack, int indent){
	for (size_t i = 0; i < taskStack.size(); i++){
		for (int j = 0; j < indent; j++) std::cout << "\t";
		std::cout << "\t\t#" << i << " " << taskStack[i] << " " << vertex_to_method[methodStack[i]] << std::endl;
	}
}


int global_id_counter = 0;

struct reconstructed_plan {
	bool success;

	int root;
	
	std::deque<std::pair<int,int>> primitive_plan;
	std::deque<std::tuple<int,int,int,int,int>> abstract_plan;
	

	std::deque<int> currentStack;

	BDD endState;

	void printPlan(Model * htn){
		if (currentStack.size()){
			std::cout << "Stack:";
			for (int x : currentStack) std::cout << " " << x;
			std::cout << std::endl;
		}
		std::cout << "==>" << std::endl;
		for (std::pair<int,int> p : primitive_plan)
			std::cout << p.first << " " << htn->taskNames[p.second] << std::endl;
	
		std::cout << "root " << root << std::endl;
		
		for (auto & [id,task,method,task1,task2] : abstract_plan){
			std::cout << id << " " << htn->taskNames[task] << " -> " << htn->methodNames[method];
			if (task1 != -1) std::cout << " " << task1;
			if (task2 != -1) std::cout << " " << task2;
			std::cout << std::endl;
		}

		std::cout << "<==" << std::endl;
	}

	int primitiveCost (Model * htn){
		int c = 0;
		for (std::pair<int,int> p : primitive_plan)
			c += htn->actionCosts[p.second];
		return c;
	}
};


reconstructed_plan get_fail(){
	reconstructed_plan r;
	r.success = false;
	return r;
}

reconstructed_plan get_empty_success(BDD state){
	reconstructed_plan r;
	r.success = true;
	r.root = global_id_counter;
	r.currentStack.push_front(global_id_counter++);
	r.endState = state;
	return r;
}


#ifndef NDEBUG
	int debug_counter = 0;
	void pc(int myCounter){
		std::cout << "===" << std::setfill('0') << std::setw(5) << myCounter << "===";
	}
#endif


// forward declaration
reconstructed_plan extract2(int curCost, int curDepth, int curTask, int curTo,
	int targetTask,
	int targetCost,
	BDD targetState,
	std::deque<int>  taskStack,
	std::deque<int>  methodStack,
	BDD curState,
	int method,
	Model * htn,
	symbolic::SymVariables & sym_vars,
	std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >> & prim_q,
	std::map<int,std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >>> & abst_q,
	std::vector<std::map<int,std::map<int,std::vector<tracingInfo>> >> & eps_inserted
		);


reconstructed_plan extract2From(int curCost, int curDepth, int curTask, int curTo,
	int targetTask,
	int targetCost,
	BDD targetState,
	std::deque<int>  taskStack,
	std::deque<int>  methodStack,
	BDD & curState,
	Model * htn,
	symbolic::SymVariables & sym_vars,
	std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >> & prim_q,
	std::map<int,std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >>> & abst_q,
	std::vector<std::map<int,std::map<int,std::vector<tracingInfo>> >> & eps_inserted
		){
#ifndef NDEBUG
	int mc = debug_counter++;
#endif	
	assert(taskStack.size() == methodStack.size());
	
	DEBUG(pc(mc); std::cout << "\t" << color(YELLOW,"Extract from ") << curCost << " " << curDepth << ": " << curTask << " " << vertex_to_method[curTo] << " stack size: " << taskStack.size() << " target " << targetTask << " target cost " << targetCost << std::endl);
	if (targetCost != -1 && curCost < targetCost){
		DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"too expensive") << std::endl);
		//exit(0);
		return get_fail();
	}
	
	if (targetTask != -1 && taskStack.size() == 1 && taskStack[0] == targetTask){
		if (!(targetState * curState).IsZero()){
			DEBUG(pc(mc); std::cout << "\t\t" << color(GREEN,"Got to target task on stack.") << std::endl);
			return get_empty_success(curState * targetState);
		} else {
			DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Got to target task on stack, but state does not fit.") << std::endl);
	  		//sym_vars.bdd_to_dot(curState, "curStateFail.dot");
			//exit(0);
			return get_fail();
		}
	}
	
	// look at my sources
	std::tuple<int,int> tup = {curTask, curTo};
	auto & myPredecessors = (curDepth == 0) ? prim_q[curCost][tup] : abst_q[curCost][curDepth][tup];
	DEBUG(
			pc(mc); std::cout << "\tOptions: " << myPredecessors.size() << std::endl;
			for (auto & [preCost, preDepth, preTask, preTo, _method, cost, getHere, _] : myPredecessors){
				pc(mc);
				std::cout << "\t\tEdge " << " " << preTask << " " << (preTo != -1 ? vertex_to_method[preTo] : -1) << std::endl;
			}
		);
	
	for (auto & [preCost, preDepth, preTask, preTo, _method, cost, depth, extraCost] : myPredecessors){
		DEBUG(pc(mc); std::cout << "\t\t" << color(YELLOW, "Trying Edge ") << preCost << " " << preDepth << " " << preTask << " " << (preTo != -1 ? vertex_to_method[preTo] : -1) << std::endl);
		
		int appliedMethod = _method;
	
	
		if (cost != -1 && depth >= 0){
			DEBUG(pc(mc); std::cout << "\t\t" << color(YELLOW,"This edge was inserted due to an epsilon application.") << " cost=" << cost << " depth=" << depth << " delta=" << extraCost << " m=" << appliedMethod << std::endl);

			int toVertex = methods_with_two_tasks_vertex[appliedMethod];
			int abstractTask = htn->decomposedTask[appliedMethod];

			assert(preTask == abstractTask);
			
			DEBUG(pc(mc); std::cout << "\t\t\tTask on the main edge is " << htn->taskNames[abstractTask] << " (#" << abstractTask << ")" << std::endl);
			DEBUG(pc(mc); std::cout << "\t\t\t\tOptions:" << eps_inserted[toVertex][cost][depth].size() << std::endl);

			int zeroInter = 0;

			for (auto & [preCost2, preDepth2, preTask2, preTo2, _method2, remainingTask, remainingMethod, _] : eps_inserted[toVertex][cost][depth]){
				DEBUG(pc(mc); std::cout << "\t\t\t" << preCost2 << " " << preDepth2 << " " << preTask2 << " " << preTo2 << " " << _method2 << " " << remainingTask << " " << vertex_to_method[remainingMethod] << " " << endl);
			
				// start reconstruction with empty stack	
				std::deque<int> ss;
				std::deque<int> sm;

				ss.push_back(remainingTask);
				sm.push_back(remainingMethod);

				
				BDD edgeBDD = eps[toVertex][cost][depth];
				//edgeBDD = edgeBDD.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsAux);


				BDD nextTargetState = curState * edgeBDD;
				if (nextTargetState.IsZero()){
					std::cout << "Intersection is Zero!" << std::endl;
					zeroInter++;
					continue;
					exit(0);
				}
				nextTargetState = nextTargetState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsPre);
				nextTargetState = nextTargetState.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff);
				
				if (nextTargetState.IsZero()){
					std::cout << "Target state is Zero!" << std::endl;
					exit(0);
				}

	  			//sym_vars.bdd_to_dot(edgeBDD, "edgeBDD.dot");
	  			//sym_vars.bdd_to_dot(nextTargetState, "nextTargetState.dot");
	  			//sym_vars.bdd_to_dot(curState, "curStateStart.dot");
				
				
				reconstructed_plan a = extract2(preCost2, preDepth2, preTask2, preTo2, abstractTask, cost - extraCost, nextTargetState, ss, sm, curState, _method2, htn, sym_vars, prim_q, abst_q, eps_inserted);
				
				if (a.success){
					if (a.primitiveCost(htn) != preCost2 - cost + extraCost){
						DEBUG(pc(mc)); std::cout << "Epsilon: I was expecting " << preCost2 - cost << " " << preCost2 << " " << cost << " but got " << a.primitiveCost(htn) << std::endl;
						exit(0);	
					} else {
						DEBUG(pc(mc)); std::cout << "\t\t\tEpsilon: I got the correct cost " << a.primitiveCost(htn) << " going on " << preCost << " " << preDepth << " " << preTask << " " << sm.front() << std::endl;
					}
					// apply the method
					
					ss = taskStack;
					sm = methodStack;
					
					ss.pop_front();
					ss.push_front(abstractTask);
				
					DEBUG(printStack(ss,sm,8));
					assert(ss.front() == preTask);
	
					reconstructed_plan b = extract2From(preCost, preDepth, ss.front(), sm.front(), targetTask, targetCost, targetState,
							ss, sm, a.endState, htn, sym_vars, prim_q, abst_q, eps_inserted);
					
					if (b.success) {
						//a.printPlan(htn);
						//b.printPlan(htn);
						
						for (auto x : a.primitive_plan)
							b.primitive_plan.push_back(x);
						a.primitive_plan = b.primitive_plan;
						for (auto x : b.abstract_plan)
							a.abstract_plan.push_back(x);
						
						// the one decomposition we do here
	
						assert(a.currentStack.size() == 1);
						assert(b.currentStack.size() >= 1);
						int aRoot = a.root;
						int aStackTop = a.currentStack.front();
						int bStackTop = b.currentStack.front();
						// replace 
						for (auto & x : a.primitive_plan)
							if (get<0>(x) == aRoot)
								get<0>(x) = bStackTop;
						for (auto & x : a.abstract_plan)
							if (get<0>(x) == aRoot)
								get<0>(x) = bStackTop;
	
	
						a.currentStack = b.currentStack;
						a.currentStack.pop_front();
						a.currentStack.push_front(aStackTop);
						
						a.root = b.root;
					
						//a.printPlan(htn);
	
						return a;
					}
				}
			}

			if (eps_inserted[toVertex][cost][depth].size() == zeroInter){
				std::cout << "No intersection in any option!" << std::endl;
				exit(0);
			}
			continue;
		}
		
		if (cost != -1 && depth < 0){
			exit(0);
			int toVertex = methods_with_two_tasks_vertex[appliedMethod];
			int abstractTask = htn->decomposedTask[appliedMethod];

			for (auto & [extraCost,extraDepth] : edgesExtraCostSource[toVertex][tasks_per_method[appliedMethod].second][-depth-1][cost]){
				int additionCost = preCost - extraCost;
				if (additionCost < 0) continue;
				
				DEBUG(pc(mc); std::cout << "\t\t" << color(YELLOW,"This edge carries additional cost.") << " cost=" << additionCost << " m=" << appliedMethod << " " << htn->methodNames[appliedMethod] << std::endl);
				DEBUG(pc(mc); std::cout << "Cost I am expecting " << curCost << " " << preCost << " " << extraCost << " " << cost << std::endl);
	
				// first part: extract the plan that decomposed 
				std::deque<int> ss;
				std::deque<int> sm;

				int firstTask = tasks_per_method[appliedMethod].first;
				// the state I am going to next ...
				BDD nextTargetState = edges[0][abstractTask][methodStack.front()][extraCost][extraDepth].AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
				reconstructed_plan a = extract2(preCost, preDepth, preTask, preTo, firstTask, preCost - additionCost, nextTargetState, ss, sm, curState, _method, htn, sym_vars, prim_q, abst_q, eps_inserted);
				
				if (a.success && a.primitiveCost(htn) != additionCost){
					DEBUG(pc(mc));
					std::cout << "Cost I was expecting " << additionCost << " but got " << a.primitiveCost(htn) << std::endl;
					exit(0);
				}

				
				if (a.success && a.primitiveCost(htn) == additionCost){
					ss = taskStack;
					sm = methodStack;
							
					ss.pop_front();
					ss.push_front(abstractTask);


					DEBUG(pc(mc); std::cout << "\t\t" << color(MAGENTA,"Main continuation ") << extraCost << " " << extraDepth << std::endl);
					reconstructed_plan b = extract2From(extraCost, extraDepth, ss.front(), sm.front(), targetTask, targetCost, targetState,
							ss, sm, a.endState, htn, sym_vars, prim_q, abst_q, eps_inserted);
					
					if (b.success){
						//a.printPlan(htn);
						//b.printPlan(htn);
						
						for (auto x : a.primitive_plan)
							b.primitive_plan.push_back(x);
						a.primitive_plan = b.primitive_plan;

						for (auto x : b.abstract_plan)
							a.abstract_plan.push_back(x);
						
						// the one decomposition we do here
	
						assert(a.currentStack.size() == 0);
						assert(b.currentStack.size() >= 1);
						int aRoot = a.root;
						
						int sub2 = global_id_counter++;
						int taskToDecompose = b.currentStack.front();
						a.abstract_plan.push_back({taskToDecompose, htn->decomposedTask[appliedMethod], appliedMethod, a.root, sub2});
	
						a.currentStack = b.currentStack;
						a.currentStack.pop_front();
						a.currentStack.push_front(sub2);
						
						a.root = b.root;
						//a.printPlan(htn);
						return a;
					}
				}
			}
			continue;
		}

		if (preCost == curCost){
			// the current layer is an abstract one. Then going to the previous layer internal parts of the stack may vanish.
			BDD state = curState; // stack state, note that this is not the state we are currently in, but the one that is used to connect valid stacks
			int currentVertex = 0;
			int lastUnmatchingStack = -1;
			for (size_t i = 0; i < methodStack.size(); i++){
				DEBUG(pc(mc); std::cout << "\t\t\t\t\t\t\t\t\tStack (2) pos " << i << "/" << methodStack.size() << std::endl);
				int taskToGo = taskStack[i];
				int targetVertex = methodStack[i];
				
				ensureBDD(currentVertex,taskToGo,targetVertex,curCost,curDepth,sym_vars); // maybe there isn't even a BDD
				BDD transitionBDD = edges[currentVertex][taskToGo][targetVertex][curCost][curDepth];
				BDD nextBDD;
				if (i != 0) // state is in v'
					state = state.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff);
				
				nextBDD = transitionBDD * state; // v'' contains state after the edge
				
				if (i != 0){
					ensureBDD(currentVertex,taskToGo,targetVertex,curCost,curDepth-1,sym_vars); // maybe there isn't even a BDD
					BDD edgeBDD = edges[currentVertex][taskToGo][targetVertex][curCost][curDepth-1];
					if (edgeBDD.IsZero()){
						DEBUG(pc(mc); std::cout << "\t\t\t" << color(RED,"stack element not present in previous round (edge not there) ") << vertex_to_method[currentVertex] << " " << 
							taskToGo << " " << vertex_to_method[targetVertex] << std::endl);
						lastUnmatchingStack = i;	
					}
					BDD presentInPrevious =  edgeBDD * nextBDD;
					if (presentInPrevious.IsZero()){
						DEBUG(pc(mc); std::cout << "\t\t\t" << color(RED,"stack element not present in previous round (state not traversable) ") << vertex_to_method[currentVertex] << " " << 
							taskToGo << " " << vertex_to_method[targetVertex] << std::endl);
						lastUnmatchingStack = i;	
					}
				}
				
				nextBDD = nextBDD.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsAux);
				nextBDD = nextBDD.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
	
				state = nextBDD;
				currentVertex = targetVertex;
			}

			if (lastUnmatchingStack != -1){
				// split the stack in two and call separately
				DEBUG(pc(mc); printStack(taskStack, methodStack, 2));
				DEBUG(pc(mc); std::cout << color(YELLOW,"\t\tStack does not match") << ", splitting at position " << lastUnmatchingStack << std::endl);
				int appliedMethod = vertex_to_method[methodStack[lastUnmatchingStack-1]]; 
				DEBUG(pc(mc); std::cout << "\t\t\tApplied method is " << htn->methodNames[appliedMethod] << " (#" << appliedMethod << ")" << std::endl);

				std::deque<int> firstTaskStack;
				std::deque<int> firstMethodStack;
				std::deque<int> secondTaskStack;
				std::deque<int> secondMethodStack;
				secondTaskStack.push_back(htn->decomposedTask[appliedMethod]);
				secondMethodStack.push_back(methodStack[lastUnmatchingStack]);

				for (size_t i = 0; i < methodStack.size(); i++){
					if (i < lastUnmatchingStack){
						firstTaskStack.push_back(taskStack[i]);
						firstMethodStack.push_back(methodStack[i]);
					} else if (i > lastUnmatchingStack){
						secondTaskStack.push_back(taskStack[i]);
						secondMethodStack.push_back(methodStack[i]);
					}
				}
				
				DEBUG(
					pc(mc); std::cout << "\t\t\tStack 1:" << std::endl;
					pc(mc); printStack(firstTaskStack, firstMethodStack, 2);
					pc(mc); std::cout << "\t\t\tStack 2:" << std::endl;
					pc(mc); printStack(secondTaskStack, secondMethodStack, 2);
					pc(mc); std::cout << "\t\t\tHead task of method " << tasks_per_method[appliedMethod].first << std::endl;
					);
				
				
				reconstructed_plan a = extract2From(curCost, curDepth, firstTaskStack.front(), firstMethodStack.front(), tasks_per_method[appliedMethod].first, curCost,
						curState, firstTaskStack, firstMethodStack, curState, htn, sym_vars, prim_q, abst_q, eps_inserted);
				
				if (a.success) {
					reconstructed_plan b = extract2From(curCost, curDepth, secondTaskStack.front(), secondMethodStack.front(), targetTask, targetCost,
							targetState, secondTaskStack, secondMethodStack, curState, htn, sym_vars, prim_q, abst_q, eps_inserted);
				
					if (b.success) {
						for (auto x : b.primitive_plan)
							a.primitive_plan.push_back(x);
						for (auto x : b.abstract_plan)
							a.abstract_plan.push_back(x);
						
						// the one decomposition we do here
						int taskToDecompose = b.currentStack.front(); b.currentStack.pop_front();
						int sub2 = global_id_counter++;
						a.abstract_plan.push_back({taskToDecompose, htn->decomposedTask[appliedMethod], appliedMethod, a.root, sub2});
						
						a.currentStack.push_back(sub2);
						for (auto x : b.currentStack)
							a.currentStack.push_back(x);
						
						a.root = b.root;
						return a;
					}
				}
				continue; // can't use this else ..
			}
		}

		reconstructed_plan a = extract2(preCost, preDepth, preTask, preTo, targetTask, targetCost, targetState, taskStack, methodStack, curState, _method, htn, sym_vars, prim_q, abst_q, eps_inserted);
		if (a.success) return a;
	}

	DEBUG(pc(mc); std::cout << color(YELLOW,"Backtracking failed at this point ... ") << std::endl);
	return get_fail();
}


////////////// CORRECTS THE STACK TO FIT TO CURRENT LAYER
reconstructed_plan extract2(int curCost, int curDepth, int curTask, int curTo,
	int targetTask,
	int targetCost,
	BDD targetState,
	std::deque<int>  taskStack,
	std::deque<int>  methodStack,
	BDD curState,
	int method,
	Model * htn,
	symbolic::SymVariables & sym_vars,
	std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >> & prim_q,
	std::map<int,std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >>> & abst_q,
	std::vector<std::map<int,std::map<int,std::vector<tracingInfo>> >> & eps_inserted
		){
#ifndef NDEBUG
	int mc = debug_counter++;
#endif
	assert(taskStack.size() == methodStack.size());
	
	if (curCost == -1){
		if (!(targetState * curState).IsZero()){
			DEBUG(pc(mc); std::cout << "DONE" << std::endl);
			return get_empty_success(curState * targetState);
		} else {
			DEBUG(pc(mc); std::cout << "Target state does not fit" << std::endl);
			return get_fail();
		}
	}
	
	DEBUG(
		pc(mc); std::cout << "Extracting solution starting cost=" << curCost << " depth=" << curDepth << " t=" << curTask << " m=" << vertex_to_method[curTo] << std::endl;
		pc(mc);
	   	if (curTask < htn->numActions) std::cout << "\tPRIM " << color(GREEN,htn->taskNames[curTask]) << " c=" << htn->actionCosts[curTask] << std::endl;
		else                           std::cout << "\tABST " << color(BLUE,htn->taskNames[curTask]) << " method " << color(CYAN, htn->methodNames[method]) << 
													" #" << method << std::endl;
		);

	// output the stack
	assert(taskStack.size() == methodStack.size());

	///////////////////////////////////////// can we do this edge backwards?
	// state contains in v the current state and in v'' something
	BDD previousState; // compute this
	if (curTask < htn->numActions) {
		// do a pre-image
		previousState = trs[curTask].preimage(curState);
	} else {
		// abstract, state does not change
		previousState = curState;
	}

	// check whether we can actually come from here
	BDD possibleSourceState = previousState * edges[0][curTask][curTo][curCost][curDepth];
	possibleSourceState = possibleSourceState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff); // remove the effect variables
	possibleSourceState = possibleSourceState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux); // remove the auxiliary variables
	// possible source state is in v, this is the state at vertex 0


	if (possibleSourceState.IsZero()) {
		DEBUG(pc(mc); std::cout << color(RED,"\t\tBacktracking, state does not fit.") << std::endl);
		return get_fail();
	}

	// reconstruct how the stack would look like if we did this push
	
	if (curTask < htn->numActions) {
		// true action
		if (false && methodStack.size()) { // we can't do this check for the first edge
			// check whether we could have gone these two edges
			BDD edge1BDD = possibleSourceState.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);
			BDD edge2BDD = edges[curTo][taskStack.front()][methodStack.front()][curCost][curDepth];

			possibleSourceState = edge1BDD * edge2BDD;
			possibleSourceState = possibleSourceState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);
		
			if (possibleSourceState.IsZero()){	
				DEBUG(pc(mc); std:: cout << color(RED,"\tTransition from new edge to previous one is not possible.") << std::endl);
				return get_fail();
			}
		}

		// push the state onto the stack
		methodStack.push_front(curTo);
		taskStack.push_front(curTask);
	} else {
		if (htn->numSubTasks[method] == 0){
			methodStack.push_front(curTo);
			taskStack.push_front(curTask);
		} else if (htn->numSubTasks[method] == 1){
			if (taskStack[0] != htn->subTasks[method][0]){
				DEBUG(pc(mc); std:: cout << color(RED,"\tFirst task on stack does not match method.") << std::endl);
				return get_fail();
			}

			// state is already checked above
			
			taskStack.pop_front();		
			taskStack.push_front(curTask);
		} else if (htn->numSubTasks[method] == 2){ // something else cannot happen
			DEBUG(pc(mc); std::cout << "\tMethod expecting " << tasks_per_method[method].first << " " << tasks_per_method[method].second << " M " << vertex_to_method[curTo] << std::endl);
			
			if (taskStack[0] != tasks_per_method[method].first){
				DEBUG(pc(mc); std:: cout << color(RED,"\tFirst task on stack does not match method.") << std::endl);
				return get_fail();
			}
		
			int tasksToCleanup = 1;
			
			// check whether this method actually does to the vertex we are expecting
			if (taskStack[1] != tasks_per_method[method].second || curTo != methodStack[1]){
				DEBUG(pc(mc); 
					if (taskStack[1] != tasks_per_method[method].second)
						std::cout << color(RED,"\tSecond task on stack does not match method.") << std::endl;
					else 
						std::cout << color(RED,"\tCan't go to ") << vertex_to_method[curTo] << " on stack is " << vertex_to_method[methodStack[1]] << std::endl;
					);
				
				DEBUG(pc(mc); std::cout << color(RED,"\tStack cleanup impossible.") << std::endl);
				return get_fail();
			}
			

			// check whether this edge is compatible with the states as we know them
			if (false){
				// check whether we could have gone these two edges
				BDD edge1BDD = possibleSourceState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff).SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsAux);
				BDD edge2BDD = edges[0][curTask][curTo][curCost][curDepth];

				possibleSourceState = edge1BDD * edge2BDD;
				possibleSourceState = possibleSourceState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);
				
				DEBUG(pc(mc); cout << "\t\t\t\t##################### 2 " << possibleSourceState.IsZero() << endl);
				
				if (possibleSourceState.IsZero()){
					return get_fail();
				}
			}
		
			taskStack.pop_front();
			
			for (int i = 0; i < tasksToCleanup; i++){
				taskStack.pop_front();
				methodStack.pop_front();
			}
			
			taskStack.push_front(curTask);
		}
	}
	
	DEBUG(
		pc(mc); std::cout << "\tStack (after modification): " << taskStack.size() << std::endl;
		printStack(taskStack,methodStack,0)
	);
	
	assert(taskStack.size() == methodStack.size());

	// check at this point whether the stack can actually look as we constructed
	BDD state = previousState; // stack state, note that this is not the state we are currently in, but the one that is used to connect valid stacks
	int currentVertex = 0;
	for (size_t i = 0; i < methodStack.size(); i++){
		DEBUG(pc(mc); std::cout << "\t\t\t\t\t\t\tStack pos " << i << "/" << methodStack.size() << std::endl); 
		int taskToGo = taskStack[i];
		int targetVertex = methodStack[i];
		DEBUG(pc(mc); std::cout << "\t\t\t\t\t\t\t\t " << vertex_to_method[currentVertex] << " " << taskToGo << " " << vertex_to_method[targetVertex] << " " << curCost << " " << curDepth << std::endl); 
		ensureBDD(currentVertex,taskToGo,targetVertex,curCost,curDepth,sym_vars);
		BDD transitionBDD = edges[currentVertex][taskToGo][targetVertex][curCost][curDepth];
		BDD nextBDD;
		if (i != 0) // state is in v'
			state = state.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff);
		
		
		state.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsAux);
		transitionBDD.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsAux);
		
		nextBDD = transitionBDD * state; // v'' contains state after the edge
		nextBDD = nextBDD.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsAux);
		nextBDD = nextBDD.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);

		state = nextBDD;
		currentVertex = targetVertex;
		if (state.IsZero()){
			DEBUG(pc(mc); std::cout << color(RED,"\t\tNot a valid stack state.") << std::endl);
			return get_fail();
		}
	}

	// TODO: maybe the stack tracing has ruled out some of the states????

	reconstructed_plan a = extract2From(curCost, curDepth, curTask, curTo, targetTask, targetCost, targetState, taskStack, methodStack, possibleSourceState, htn, sym_vars, prim_q, abst_q, eps_inserted);

	if (!a.success) return a;

	if (curTask < htn->numActions) {
		int stackTask = a.currentStack.front(); a.currentStack.pop_front();
		// add a primitive action
		a.primitive_plan.push_back({stackTask,curTask});
	} else {
		if (htn->numSubTasks[method] == 1){
			int stackTask = a.currentStack.front(); a.currentStack.pop_front();
			int sub1 = global_id_counter++;

			a.abstract_plan.push_back({stackTask, curTask, method, sub1, -1});
			a.currentStack.push_front(sub1);
		} else if (htn->numSubTasks[method] == 2){ // something else cannot happen
			int stackTask = a.currentStack.front(); a.currentStack.pop_front();
			int sub1 = global_id_counter++;
			int sub2 = global_id_counter++;

			a.abstract_plan.push_back({stackTask, curTask, method, sub1, sub2});
			a.currentStack.push_front(sub2);
			a.currentStack.push_front(sub1);
		} else {
			int stackTask = a.currentStack.front(); a.currentStack.pop_front();
			a.abstract_plan.push_back({stackTask, curTask, method, -1, -1});
		}
	}


	//a.printPlan(htn);
	DEBUG(std::cout << a.primitiveCost(htn) << " " << curCost << " " << targetCost << std::endl);
	if (a.primitiveCost(htn) != curCost - targetCost){
		std::cout << color(RED,"Does not fit") << std::endl;
	}
	assert(a.primitiveCost(htn) == curCost - targetCost);
	//	return get_fail();

	return a;
}



void extract(int curCost, int curDepth, int curTask, int curTo,
	BDD state,
	int method, 
	Model * htn,
	symbolic::SymVariables & sym_vars,
	std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >> & prim_q,
	std::map<int,std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >>> & abst_q,
	std::vector<std::map<int,std::map<int,std::vector<tracingInfo>> >> & eps_inserted
	){
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << "========================================================================" << std::endl;

	std::deque<int> ss;
	std::deque<int> sm;

	state = state.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff); // remove the effect variables
	state = state.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux); // remove the auxiliary variables
	
	BDD init = sym_vars.getStateBDD(htn->s0List, htn->s0Size);
	
	std::clock_t plan_start = std::clock();
	reconstructed_plan a = extract2(curCost, curDepth, curTask, curTo, -1, 0, init, ss, sm, state, method, htn, sym_vars, prim_q, abst_q, eps_inserted);
	std::clock_t plan_end = std::clock();
	double plan_time_in_ms = 1000.0 * (plan_end-plan_start) / CLOCKS_PER_SEC;
	std::cout << "Plan reconstruction time: " << fixed << plan_time_in_ms  << "ms " << fixed << plan_time_in_ms/1000 << "s " << fixed << plan_time_in_ms / 60000 << "min" << std::endl << std::endl;
	
	/// output the found plan
	if (a.success) std::cout << color(GREEN,"Extracted plan") << std::endl;
	else           std::cout << color(RED,  "Plan extraction failed") << std::endl;
	
	if (a.success) a.printPlan(htn);

	// just checking ...
	std::cout << "Cost: search=" << curCost << " extracted=" << a.primitiveCost(htn) << std::endl;
	exit(0);
}


//================================================== planning algorithm

void build_automaton(Model * htn){
	std::clock_t preparation_start = std::clock();
	std::cout << "Building transition relations" << std::endl;
	
	// Symbolic Playground
	symbolic::SymVariables sym_vars(htn);
	sym_vars.init(true);
	BDD init = sym_vars.getStateBDD(htn->s0List, htn->s0Size);
	BDD goal = sym_vars.getPartialStateBDD(htn->gList, htn->gSize);
	
	for (int i = 0; i < htn->numActions; ++i) {
	  DEBUG(std::cout << "Creating TR " << i << std::endl);
	  trs.emplace_back(&sym_vars, i, htn->actionCosts[i]);
	  trs.back().init(htn);
	  //sym_vars.bdd_to_dot(trs.back().getBDD(), "op" + std::to_string(i) + ".dot");
	}



	// actual automaton construction
	std::vector<int> methods_with_two_tasks;
	// the vertex number of these methods, 0 and 1 are start and end vertex
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
	edgesExtraCost.resize(number_of_vertices);
	edgesExtraCostSource.resize(number_of_vertices);

	// build the initial version of the graph
	vertex_names.push_back("start");
	vertex_names.push_back("end");
	for (auto & [method, vertex] : methods_with_two_tasks_vertex){
		vertex_names.push_back(htn->methodNames[method]);

		// which one is the second task of this method
		int first = htn->subTasks[method][0];
		int second = htn->subTasks[method][1];
		assert(htn->numOrderings[method] == 2);
		if (htn->ordering[method][0] == 1 && htn->ordering[method][1])
			std::swap(first,second);

		tasks_per_method[method] = {first, second};
	}


	// add the initial abstract task
	edges[0][htn->initialTask][1][0][0] = init;



	// apply transition rules
	
	// loop over the outgoing edges of 0, only to those rules can be applied

	std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >> prim_q;
	std::map<int,std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >>> abst_q;
	std::vector<std::map<int,std::map<int,std::vector<tracingInfo>> >> eps_inserted (number_of_vertices);
	std::vector<std::map<int,BDD>> state_expanded_at_cost;

	std::map<int,std::map<int,BDD>> _empty_eps;
	_empty_eps[0][0] = sym_vars.zeroBDD();
	for (int i = 0; i < number_of_vertices; i++) eps.push_back(_empty_eps);
	std::map<int,BDD> _empty_state;
	_empty_state[0] = sym_vars.zeroBDD();
	for (int i = 0; i < number_of_vertices; i++) state_expanded_at_cost.push_back(_empty_state);

#define put push_back
//#define put insert

	std::tuple<int,int> _tup = {htn->initialTask, 1};
	tracingInfo _from = {-1,-1,-1,-1,-1,-1,-1,-1};
	abst_q[0][1][_tup].push_back(_from);
		
	std::clock_t preparation_end = std::clock();
	double preparation_time_in_ms = 1000.0 * (preparation_end-preparation_start) / CLOCKS_PER_SEC;
	std::cout << "Preparation time: " << fixed << preparation_time_in_ms << "ms " << fixed << preparation_time_in_ms/1000 << "s " << fixed << preparation_time_in_ms / 60000 << "min" << std::endl << std::endl;

	// cost of current layer and whether we are in abstract or primitive mode
	int currentCost = 0;
	int currentDepthInAbstract = 0; // will be zero for the primitive round
	
	auto addQ = [&] (int task, int to, int extraCost, int fromTask, int fromTo, int method, int cost, int depth, bool insertOnlyIfNonEmpty) {
		std::tuple<int,int> tup = {task, to};
		tracingInfo from = {currentCost, currentDepthInAbstract, fromTask, fromTo, method, cost, depth, extraCost};

		std::vector<tracingInfo> * insertQueue;
#ifndef NDEBUG
		int insertCost = -1, insertDepth = -1;
#endif
		if (task >= htn->numActions || htn->actionCosts[task] == 0){
			// abstract task or zero-cost action
			if (!extraCost) {
				insertQueue = &(abst_q[currentCost][currentDepthInAbstract+1][tup]);
				DEBUG(insertCost = currentCost; insertDepth = currentDepthInAbstract + 1);
			} else {
				insertQueue = &(abst_q[currentCost + extraCost][1][tup]);
				DEBUG(insertCost = currentCost + extraCost; insertDepth = 1);
			}
		} else {
			// primitive with cost
			insertQueue = &(prim_q[currentCost + extraCost + htn->actionCosts[task]][tup]);
			DEBUG(insertCost = currentCost + extraCost + htn->actionCosts[task]; insertDepth = 0);
		}

		if (!insertOnlyIfNonEmpty || insertQueue->size() || (task < htn->numActions && htn->actionCosts[task] != 0)){
			DEBUG(std::cout << "\t\t\t\t\t\t" << color(GREEN,"Inserting ") << task << " " << vertex_to_method[to] << " to " << insertCost << " " << insertDepth << std::endl);
			insertQueue->push_back(from);
		}
	};



	std::clock_t layer_start = std::clock();
	int lastCost;
	int lastDepth;

	int step = 0;
	std::map<std::tuple<int,int>, std::vector<tracingInfo> > & current_queue = prim_q[0];
	// TODO separate trans and rel
	while (true){ // TODO: detect unsolvability
		for (auto & entry : current_queue){
			int task = get<0>(entry.first);
			int to   = get<1>(entry.first);
			
			if (entry.second.size() == 0) {
				std::cout << "Ignoring " << task << " " << vertex_to_method[to] << std::endl;
				continue;
			}
		

			BDD state = edges[0][task][to][lastCost][lastDepth];
			//sym_vars.bdd_to_dot(state, "state" + std::to_string(step) + ".dot");
	  

#ifndef NDEBUG
			if (step % 1 == 0){
#else
			if (step % 10000 == 0){
#endif
				std::cout << color(BLUE,"STEP") << " #" << step << ": " << task << " " << vertex_to_method[to];
	   			std::cout << "\t\tTask: " << htn->taskNames[task] << std::endl;		
			}
			
			if (state == sym_vars.zeroBDD()) {
				std::cout << color(RED,"state attached to edge is empty ... bug!") << std::endl;
				exit(0);
				continue; // impossible state, don't treat it
			}

			if (task < htn->numActions || htn->emptyMethod[task] != -1){
				BDD nextState = state;
				
				if (task < htn->numActions){
					DEBUG(std::cout << "Prim: " << htn->taskNames[task] << std::endl);
					// apply action to state
					nextState = trs[task].image(state);
				} else {
					DEBUG(std::cout << "Empty Method: " << htn->taskNames[task] << std::endl);
				}
				
				nextState = nextState.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);

				// check if already added
				// TODO: there is a bit faster code here
 				BDD disjunct = eps[to][currentCost][currentDepthInAbstract] + nextState;  
				if (disjunct != eps[to][currentCost][currentDepthInAbstract]){
					eps[to][currentCost][currentDepthInAbstract] = disjunct;
					DEBUG(std::cout << "\t\t\t\t\t\t" << color(MAGENTA,"Adding to eps of ") << vertex_to_method[to] << std::endl);
					

					for (auto & [task2,tos] : edges[to]){
						for (auto & [to2,bdds] : tos){
							if (!bdds.count(lastCost) || !bdds[lastCost].count(lastDepth)) continue;
							DEBUG(std::cout << "\tContinuing edge: " << task2 << " " << vertex_to_method[to2] << std::endl);
							
							BDD bdd = bdds[lastCost][lastDepth];
							BDD addStateWithIntermediate = nextState * bdd;
							BDD originalAddStateWithIntermediate = addStateWithIntermediate;
							BDD addState = addStateWithIntermediate.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);

							
							ensureBDD(task2, to2, currentCost, currentDepthInAbstract, sym_vars);
							BDD edgeDisjunct = edges[0][task2][to2][currentCost][currentDepthInAbstract] + addState;
							if (edgeDisjunct != edges[0][task2][to2][currentCost][currentDepthInAbstract]){
						   		// determine at which cost this is to be inserted ...
								for (auto [extraCost,extraBDD] : edgesExtraCost[to][task2][to2]){
									DEBUG(std::cout << "\t\t\tChecking for hidden extra costs: cost=" << extraCost << std::endl);
									// now see whether we actually intersect with this state
									// the state in extraBDD is in v', so we intersect with the stack push state
									BDD partWithExtraCost = addStateWithIntermediate * extraBDD;

									if (!partWithExtraCost.IsZero()){
										DEBUG(std::cout << "\t\t\tintersecting ... " << std::endl);
	
										// remove the intermediate variables
										BDD futureAddState = partWithExtraCost.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);
										// determine where we have to put this one

										DEBUG(std::cout << "\t\t\tFuture cost " << extraCost << std::endl);
										int targetCost = currentCost + extraCost;
									
										// determine where this edge will go to	
										int targetDepth = 0;
										if (extraCost == 0)
											targetDepth = currentDepthInAbstract;
										
										ensureBDD(task2, to2, targetCost, targetDepth, sym_vars); // add as an edge to the future
										BDD futureDisjunct = edges[0][task2][to2][targetCost][targetDepth] + futureAddState;
										if (edges[0][task2][to2][targetCost][targetDepth] != futureDisjunct){
											edges[0][task2][to2][targetCost][targetDepth] = futureDisjunct;
											DEBUG(std::cout << "\t\t\t++ edge: " << task2 << " " << vertex_to_method[to2] << std::endl);
											DEBUG(std::cout << "\t\t\t" << color(GREEN,"cost ") << extraCost << " -> " << targetCost << " " << targetDepth << std::endl);
											addQ(task2, to2, extraCost, task, to, vertex_to_method[to], extraCost, -to2-1, false); // TODO think about when to add ... the depth in abstract should be irrelevant?
										} else {
											addQ(task2, to2, extraCost, task, to, vertex_to_method[to], extraCost, -to2-1, true);
										}

										// cut this part away
										addStateWithIntermediate = addStateWithIntermediate * !partWithExtraCost;
										if (addStateWithIntermediate.IsZero()){
											DEBUG(std::cout << "\t\t\tNothing left ... " << std::endl);
										}
									}
								}
									

								if (addStateWithIntermediate.IsZero())
									continue;

								if (addStateWithIntermediate != originalAddStateWithIntermediate){
									addState = addStateWithIntermediate.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);
									edgeDisjunct = edges[0][task2][to2][currentCost][currentDepthInAbstract] + addState;
									if (edgeDisjunct == edges[0][task2][to2][currentCost][currentDepthInAbstract])
										continue;
								}
								
								
								edges[0][task2][to2][currentCost][currentDepthInAbstract] = edgeDisjunct;
								DEBUG(if (task < htn->numActions) std::cout << "\tPrim: "; else std::cout << "\tMethod: ";
										std::cout << task2 << " " << vertex_to_method[to2] << std::endl);
								addQ(task2, to2, 0, task, to, htn->emptyMethod[task], -1, -1, false); // no method as primitive
					
							
								tracingInfo tracingInf = {currentCost, currentDepthInAbstract, task, to, htn->emptyMethod[task], task2, to2, -1};
								eps_inserted[to][currentCost][currentDepthInAbstract].push_back(tracingInf);
							}
						}
					}


					if (to == 1){
					   	if (nextState * goal != sym_vars.zeroBDD()){
							std::cout << "Goal reached! Length=" << currentCost << " steps=" << step << std::endl;
	  						// sym_vars.bdd_to_dot(nextState, "goal.dot");
							
							std::clock_t search_end = std::clock();
							double search_time_in_ms = 1000.0 * (search_end-preparation_end) / CLOCKS_PER_SEC;
							std::cout << "Search time: " << fixed << search_time_in_ms << "ms " << fixed << search_time_in_ms / 1000 << "s " << fixed << search_time_in_ms / 60000 << "min" << std::endl << std::endl;
							
							extract(currentCost, currentDepthInAbstract, task, to, nextState * goal, htn->emptyMethod[task], htn, sym_vars, prim_q, abst_q, eps_inserted);
							return;
						} else {
							std::cout << "Goal not reached!" << std::endl;
						}
					}
				} else {
					DEBUG(std::cout << "\t\t" << color(RED," ... state is not new") << std::endl);
				}

			} else {
				// abstract edge, go over all applicable methods	
				
				for(int mIndex = 0; mIndex < htn->numMethodsForTask[task]; mIndex++){
					int method = htn->taskToMethods[task][mIndex];
					DEBUG(std::cout << "\t" << color(MAGENTA,"Method ") << htn->methodNames[method] << " (#" << method << ")" << std::endl);

					// cases
					if (htn->numSubTasks[method] == 0)
						continue;

					if (htn->numSubTasks[method] == 1){
						// ensure that a BDD is there
						ensureBDD(htn->subTasks[method][0], to, currentCost, currentDepthInAbstract, sym_vars);
						
						// perform the operation
						BDD disjunct = edges[0][htn->subTasks[method][0]][to][currentCost][currentDepthInAbstract] + state;
						if (disjunct != edges[0][htn->subTasks[method][0]][to][currentCost][currentDepthInAbstract]){
							edges[0][htn->subTasks[method][0]][to][currentCost][currentDepthInAbstract] = disjunct;
							DEBUG(std::cout << "\t\t" << color(GREEN,"Unit: ") << htn->subTasks[method][0] << " " << vertex_to_method[to] << std::endl);
							addQ(htn->subTasks[method][0], to, 0, task, to, method, -1, -1, false);
						}
						
					} else { // two subtasks
						assert(htn->numSubTasks[method] == 2);
						// add edge (state is irrelevant here!!)
						
						DEBUG(std::cout << "\t\ttwo element method into " << tasks_per_method[method].first << " " << tasks_per_method[method].second << std::endl);
					
						// This is what we would add to the internal edge
						// we have the current state in v' and the stack state in v''
						BDD r_temp = state.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff);
						ensureBDD(methods_with_two_tasks_vertex[method], tasks_per_method[method].second, to, currentCost, currentDepthInAbstract, sym_vars);
						BDD actually_new_r_temp = r_temp * !edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract];
						BDD actually_new_r_temp_only_source = actually_new_r_temp.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
						
						ensureBDD(methods_with_two_tasks_vertex[method], tasks_per_method[method].second, to, lastCost, lastDepth, sym_vars);
						//BDD bddR = r_temp * !edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][lastCost][lastDepth];
						BDD bddR = r_temp * !edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract];

						// new state for edge to method vertex
						BDD biimp = sym_vars.oneBDD();
						for (int i = 0; i < htn->numVars; i++)
							biimp *= sym_vars.auxBiimp(i); // v_i = v_i''

						BDD ss = state.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
						ss *= biimp;

						// we have to distinguish between the states that we already know and the ones that are new
						
						// determine the already known part of the edge
						ensureBDD(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], currentCost, currentDepthInAbstract, sym_vars);
						BDD currentBDD = edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][currentCost][currentDepthInAbstract];
						BDD alreadyKnown = ss * currentBDD;

						if (/*false &&*/ !alreadyKnown.IsZero()){
							DEBUG(std::cout << "\t\tPart of the state is already known." << std::endl);
							// handle the parts of the state by the first cost when the main edge was inserted first

							for (int getHere = 0; getHere <= lastCost; getHere++){
								auto lastOfCost = edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][getHere].end();
								if (lastOfCost == edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][getHere].begin())
									continue; // edge not there yet
									
								int lastDepth = (--lastOfCost)->first;
								if (getHere == currentCost)
									lastDepth  = currentDepthInAbstract;
								
								ensureBDD(0,tasks_per_method[method].first,methods_with_two_tasks_vertex[method],getHere,lastDepth ,sym_vars);
								BDD intersectionBDD = alreadyKnown * edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][getHere][lastDepth];

								if (intersectionBDD.IsZero()) continue;

								DEBUG(std::cout << "\t\t\t" << color(YELLOW,"States first occurring with cost ") << getHere << std::endl);


								// now there are two options:
								// (1) we may have already progressed completely through this edge
								// (2) some progressions may still be remaining
								
								// case 1: epsilon
								// we have to try out the earliest time we could have gotten to an epsilon
								
								// swap around s.t. the biimp (plus state) is in v' and v''
								BDD sIntersect = intersectionBDD.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff);

								// go through the timesteps where we can potentially have achieved an epsilon transition for this

								for (int cost = getHere; cost <= lastCost; cost++){
									for (int depth = 0; depth < eps[methods_with_two_tasks_vertex[method]][cost].size(); depth++){
										if (cost == currentCost && depth == currentDepthInAbstract) break; // don't take epsilons from current layer, they will be handled elsewhere ...?
										
										// there is no reason to consider this, as there was no newly inserted epsilon state
										if (eps_inserted[methods_with_two_tasks_vertex[method]][cost][depth].size() == 0) continue; 
	
										// state after eps in v and stack push state in v'
										BDD epsCurrent = eps[methods_with_two_tasks_vertex[method]][cost][depth];
		
										// determine the state intersection, i.e. whether we actually have a possible epsilon shortcut at this state
										BDD epsilonIntersect = epsCurrent * sIntersect;

										if (epsilonIntersect.IsZero()) continue;
										DEBUG(std::cout << "\t\t\t\t" << color(YELLOW,"Can be converted to epsilon at ") << cost << " " << depth << std::endl);
									
										////////////////////////////////////////////////////////////////	
										// found epsilon transition
										int actualCost = cost - getHere;
										DEBUG(std::cout << "\t\t\t\tEpsilon with cost " << actualCost << " from " << getHere << std::endl);
										int targetCost = currentCost + actualCost;
									
										// determine where this edge will go to	
										int targetDepth = 0;
										if (actualCost == 0)
											targetDepth = currentDepthInAbstract;
										
										ensureBDD(tasks_per_method[method].second, to, targetCost, targetDepth, sym_vars); // add as an edge to the future
									
										// remove the v'
										BDD newState = epsilonIntersect.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
										newState = newState * bddR; //actually_new_r_temp;
										newState = newState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);
										//newState = newState * biimp;
										
										BDD disjunct = edges[0][tasks_per_method[method].second][to][targetCost][targetDepth] + newState;
										if (edges[0][tasks_per_method[method].second][to][targetCost][targetDepth] != disjunct){
											edges[0][tasks_per_method[method].second][to][targetCost][targetDepth] = disjunct;
											DEBUG(std::cout << "\t\t\t\tEpsilon transition is new: " << tasks_per_method[method].second << " " << vertex_to_method[to] << " cost " << actualCost << std::endl);
											addQ(tasks_per_method[method].second, to, actualCost, task, to, method, cost, depth, false); // TODO think about when to add ... the depth in abstract should be irrelevant?
										} else {
											DEBUG(std::cout << "\t\t\t\tEpsilon transition is not new: " << tasks_per_method[method].second << " " << vertex_to_method[to] << " cost " << actualCost << std::endl);
											addQ(tasks_per_method[method].second, to, actualCost, task, to, method, cost, depth, true); 
										}
										////////////////////////////////////////////////////////////////	

										// remove the part of the state that we handled from consideration
										sIntersect = sIntersect * !(epsilonIntersect.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsPre));
										if (sIntersect.IsZero()) break;
									}
									if (sIntersect.IsZero()){
										DEBUG(std::cout << "\t\t\t\t" << color(RED,"no intersection remaining.") << std::endl);
										break;
									}
								}

								
								// case 2: unfinished transitions
								// add this to the internal edge ... we will go through it at some point
								if (false && !sIntersect.IsZero()){
									DEBUG(std::cout << "\t\t\t\t" << color(RED,"intersection remaining that is not yet progressed to epsilon.") << std::endl);
									
									// for this, only the current state in which we push is interesting
									// move the new state to V' (the eff vars)
									sIntersect = sIntersect.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);
									sIntersect = sIntersect.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsPre);
								
									// remove things that are already present at this edge
									sIntersect = sIntersect * actually_new_r_temp_only_source;
									if (sIntersect.IsZero()){
										DEBUG(std::cout << "\t\t\t\t" << color(RED, "intersection is not new for outgoing edge, so no extra cost.") << std::endl);
									} else {
										// some part of the edge is not new, so we have to annotate cost to the internal edge
										DEBUG(std::cout << "\t\t\t\t" << color(MAGENTA,"Partially known: ") << tasks_per_method[method].first << " " << vertex_to_method[methods_with_two_tasks_vertex[method]] << std::endl);
										DEBUG(std::cout << "\t\t\t\t" << color(GREEN,"Cost for internal edge: ") << vertex_to_method[methods_with_two_tasks_vertex[method]] << " " << tasks_per_method[method].second << " " << vertex_to_method[to] << std::endl);
		
										int extraCost = currentCost - getHere;

										
										ensureBDDExtraCost(methods_with_two_tasks_vertex[method], tasks_per_method[method].second, to, extraCost, sym_vars);
										// add state in V'
										edgesExtraCost[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][extraCost] |= sIntersect;
										edgesExtraCostSource[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][extraCost].push_back(
												std::make_pair(currentCost, currentDepthInAbstract));
									}
								}
	

								alreadyKnown = alreadyKnown * !intersectionBDD;
								if (alreadyKnown.IsZero()){
									DEBUG(std::cout << "\t\t\t" << color(RED,"no state remaining, ending.") << std::endl);
									break;
								}
							}
						}

					
						ensureBDD(methods_with_two_tasks_vertex[method], tasks_per_method[method].second, to, currentCost, currentDepthInAbstract, sym_vars);
					
						BDD disjunct_r_temp = edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract] + r_temp;
						if (disjunct_r_temp != edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract]){
							// add the internal outgoing edge, if it is new
							edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract] = disjunct_r_temp;

							DEBUG(std::cout << "\t\t" << color(BLUE, "Internal Edge ") << vertex_to_method[methods_with_two_tasks_vertex[method]] << " " << tasks_per_method[method].second << " " << vertex_to_method[to] << std::endl);
						}
		
						// check whether we have something new here
						BDD disjunct2 = currentBDD + ss;
					   	if (disjunct2 != currentBDD){
						   edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][currentCost][currentDepthInAbstract] = disjunct2;
							
						   DEBUG(std::cout << "\t\t" << color(GREEN,"2 normal: ") << tasks_per_method[method].first << " " << vertex_to_method[methods_with_two_tasks_vertex[method]] << std::endl);
						   addQ(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], 0, task, to, method, -1, -1, false);
					 	} else {
					  		DEBUG(std::cout << "\t\t" << color(YELLOW,"Already known: ") << tasks_per_method[method].first << " " << vertex_to_method[methods_with_two_tasks_vertex[method]] << std::endl);
					  		addQ(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], 0, task, to, method, -1, -1, true);
					  	}
						
						
						// memorise at which cost we got here, in the v_i' and v'' variables
						//state_expanded_at_cost[methods_with_two_tasks_vertex[method]][currentCost] += ss.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsPre);
						
					}
				}
			}

			//if (step == 71) exit(0);
			//if (step == 204) exit(0);

			step++;
		}
		
		//graph_to_file(htn,"layer" + std::to_string(currentCost) + "_" + std::to_string(currentDepthInAbstract) +  ".dot");

		// handled everything in this layer	
		// switch to next layer	
		std::clock_t layer_end = std::clock();
		double layer_time_in_ms = 1000.0 * (layer_end-layer_start) / CLOCKS_PER_SEC;
		std::cout << "Layer time: " << fixed << layer_time_in_ms << "ms " << fixed << layer_time_in_ms / 1000 << "s " << fixed << layer_time_in_ms / 60000 << "min" << std::endl << std::endl;
		layer_start = layer_end;

		lastCost = currentCost;
		lastDepth = currentDepthInAbstract;

		// check whether we have to stay abstract
		std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >>::iterator next_abstract_layer;
		if (currentDepthInAbstract == 0){
			next_abstract_layer = abst_q[currentCost].begin();
		} else {
			next_abstract_layer = abst_q[currentCost].find(currentDepthInAbstract);
			if (next_abstract_layer != abst_q[currentCost].end()){
				DEBUG(std::cout << "Non last layer " << next_abstract_layer->first << std::endl);
				next_abstract_layer++;
				DEBUG(std::cout << "Next last layer " << next_abstract_layer->first << std::endl);
			}
		}
		
		if (next_abstract_layer == abst_q[currentCost].end()){
			// transition to primitive layer
			auto findIt = prim_q.find(currentCost);
			findIt++;
			if (findIt == prim_q.end() && ++abst_q.find(currentCost) == abst_q.end()){
				std::cout << color(RED,"Problem is unsolvable") << std::endl;
				exit(0);
			}
			currentCost = findIt->first;
			currentDepthInAbstract = 0; // the primitive layer
			current_queue = prim_q[currentCost];
			std::cout << color(CYAN,"==========================") << " Cost Layer " << currentCost << std::endl; 
			//if (currentCost == 29) exit(0);
		} else {
			currentDepthInAbstract = next_abstract_layer->first;
			current_queue = next_abstract_layer->second;
			std::cout << color(CYAN,"==========================") << " Abstract layer of cost " << currentCost << " layer: " << currentDepthInAbstract << std::endl; 
		}
		std::cout << "Size of Queue for new layer " << current_queue.size() << std::endl;

		//std::cout << "COPY " << std::endl; 

		// copy the graph for the new cost layer
		for (int from = 0; from < edges.size(); from++)
			for (auto & [task,es] : edges[from])
				for (auto & [to, bdds] : es){
					if (!bdds.count(lastCost)) continue;
					if (!bdds[lastCost].count(lastDepth)) continue;
					ensureBDD(from,task,to,currentCost,currentDepthInAbstract,sym_vars); // might already contain a BDD from epsilon transitions
					edges[from][task][to][currentCost][currentDepthInAbstract] += bdds[lastCost][lastDepth];
				}
		
		//std::cout << "edges done" << std::endl; 
		
		// epsilon set
		for (int from = 0; from < edges.size(); from++)
			eps[from][currentCost][currentDepthInAbstract] = eps[from][lastCost][lastDepth];
		
		//std::cout << "eps done" << std::endl; 
		
		if (currentCost != lastCost)
			for (int from = 0; from < edges.size(); from++){
				state_expanded_at_cost[from][currentCost] = state_expanded_at_cost[from][lastCost];
			}
		
		//std::cout << "state done" << std::endl; 
	}
	
	std::cout << "Ending ..." << std::endl;
	exit(0);
	//delete sym_vars.manager.release();






	//std::cout << to_string(htn) << std::endl;
}

