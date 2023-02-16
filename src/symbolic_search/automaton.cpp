#include "../Debug.h"
#include "../Util.h"
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


typedef std::tuple<int,int,int,int,int,int,int,int> tracingInfo;

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
std::vector<std::map<int,std::map<int, std::map<int, std::vector<std::tuple<int,int,int,int>>>>>> edgesExtraCostSource;

std::map<int,std::pair<int,int>> tasks_per_method; // first and second
	
std::vector<std::map<int,std::map<int,BDD>>> eps;
std::vector<std::map<int,std::map<int,std::vector<tracingInfo>> >> eps_inserted;

std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >> prim_q;
std::map<int,std::map<int,std::map<std::tuple<int,int>, std::vector<tracingInfo> >>> abst_q;


std::vector<symbolic::TransitionRelation> trs;


BDD biimp;



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

//#undef NDEBUG

//#define DEBUG(x) do { if (getDebugMode ()) { x; } } while (0)


//================================================== extract solution

#ifndef NDEBUG
	int debug_counter = 0;
	void pc(int myCounter){
		std::cout << "===" << std::setfill('0') << std::setw(5) << myCounter << "=== ";
	}
#endif




void printStack(
#ifndef NDEBUG
	int mc,
#endif
	std::deque<int> & taskStack, std::deque<int> & methodStack, int indent){
	for (size_t i = 0; i < taskStack.size(); i++){
#ifndef NDEBUG
		pc(mc);
#endif
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



std::deque<BDD> checkStack(
#ifndef NDEBUG
	int mc,
#endif	
	int curCost,
	int curDepth,
	std::deque<int> taskStack,
	std::deque<int> methodStack,
	std::deque<BDD> stateStack,
	Model * htn,
	symbolic::SymVariables & sym_vars
		){

	DEBUG(pc(mc); std::cout << "\t\t\t" << color(YELLOW,"Finding valid stack beginnings") << " at c=" << curCost << " d=" << curDepth << std::endl);

	BDD state = stateStack.front().AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux) * biimp.SwapVariables(sym_vars.swapVarsEff,sym_vars.swapVarsAux);
	
	int currentVertex = 0;
	for (size_t i = 0; i < methodStack.size(); i++){
		DEBUG(pc(mc); std::cout << "\t\t\t\tStack pos " << i << "/" << methodStack.size();); 
		int taskToGo = taskStack[i];
		int targetVertex = methodStack[i];
		DEBUG(std::cout << "\t " << vertex_to_method[currentVertex] << " " << taskToGo << " " << vertex_to_method[targetVertex] << std::endl); 
		ensureBDD(currentVertex,taskToGo,targetVertex,curCost,curDepth,sym_vars);
		BDD transitionBDD = edges[currentVertex][taskToGo][targetVertex][curCost][curDepth];
		if (transitionBDD.IsZero()) {
			DEBUG(pc(mc); std::cout << color(RED,"\t\t\tEdge not present.") << std::endl);
			state = sym_vars.zeroBDD();
			break;
		}
		
		// go only those transitions that are actually on the stack	
		transitionBDD *= stateStack[i];
		if (transitionBDD.IsZero()) {
			DEBUG(pc(mc); std::cout << color(RED,"\t\t\tStack state not compatible.") << std::endl);
			state = sym_vars.zeroBDD();
			break;
		}
		
	
		BDD nextBDD = transitionBDD * state; // v'' contains state after the edge
		nextBDD = nextBDD.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);
		nextBDD = nextBDD.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
		
		if (nextBDD.IsZero()) {
			DEBUG(pc(mc); std::cout << color(RED,"\t\t\tNot a valid stack state.") << std::endl);
			state = sym_vars.zeroBDD();
			break;
		}

		state = nextBDD;
		currentVertex = targetVertex;
	}

	// possible start states remain in v
	BDD possibleStartState = state.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);


	DEBUG(pc(mc); std::cout << "\t\t\t" << color(YELLOW,"Filtering the whole stack") << " at c=" << curCost << " d=" << curDepth << std::endl);
	
	
	// restrict the first one manually. We don't need to intersect with the edge as it has been done outside of us
	stateStack[0] *= possibleStartState;
	// but to be safe
	stateStack[0] *= edges[0][taskStack[0]][methodStack[0]][curCost][curDepth];

	currentVertex = methodStack[0];
	// put the result state in v'
	state = stateStack[0].AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsPre).SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);
	for (size_t i = 1; i < methodStack.size(); i++){
		DEBUG(pc(mc); std::cout << "\t\t\t\tStack pos " << i << "/" << methodStack.size();); 
		int taskToGo = taskStack[i];
		int targetVertex = methodStack[i];
		DEBUG(std::cout << "\t " << vertex_to_method[currentVertex] << " " << taskToGo << " " << vertex_to_method[targetVertex] << std::endl); 
		ensureBDD(currentVertex,taskToGo,targetVertex,curCost,curDepth,sym_vars);
		BDD transitionBDD = edges[currentVertex][taskToGo][targetVertex][curCost][curDepth];
		stateStack[i] *= transitionBDD; // go only those transitions that are actually on the stack
		stateStack[i] *= state;
		
		state = stateStack[i].AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff).SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);
		
		currentVertex = targetVertex;
	}
	// We don't need an other pass as there cannot be any non-traversable edges in the stack state.
	// If we inserted a state pair into an edge it has to have a valid continuation onto the goal state
	

	return stateStack;	
}


reconstructed_plan addToPlan(reconstructed_plan a, int task, int method, Model * htn){
	if (task < htn->numActions) {
		int stackTask = a.currentStack.front(); a.currentStack.pop_front();
		// add a primitive action
		a.primitive_plan.push_back({stackTask,task});
	} else {
		if (htn->numSubTasks[method] == 1){
			int stackTask = a.currentStack.front(); a.currentStack.pop_front();
			int sub1 = global_id_counter++;
	
			a.abstract_plan.push_back({stackTask, task, method, sub1, -1});
			a.currentStack.push_front(sub1);
		} else if (htn->numSubTasks[method] == 2){ // something else cannot happen
			int stackTask = a.currentStack.front(); a.currentStack.pop_front();
			int sub1 = global_id_counter++;
			int sub2 = global_id_counter++;
	
			a.abstract_plan.push_back({stackTask, task, method, sub1, sub2});
			a.currentStack.push_front(sub2);
			a.currentStack.push_front(sub1);
		} else {
			int stackTask = a.currentStack.front(); a.currentStack.pop_front();
			a.abstract_plan.push_back({stackTask, task, method, -1, -1});
		}
	}

	return a;
}



pair<int,int> prev(int cost, int depth){
	if (depth != 0)
		return {cost, depth-1};

	if (abst_q[cost-1].size() == 0)
		return {cost-1, 0};
	
	return {cost-1, (--abst_q[cost-1].end())->first};
}


////////////// CORRECTS THE STACK TO FIT TO CURRENT LAYER
bool generatePredecessor(int curTask, int curTo,
	std::deque<int> & taskStack,
	std::deque<int> & methodStack,
	std::deque<BDD> & stateStack,
	int method,
	Model * htn,
	symbolic::SymVariables & sym_vars
		){
#ifndef NDEBUG
	int mc = debug_counter++;
#endif
	assert(taskStack.size() == methodStack.size());
	assert(taskStack.size() == stateStack.size());
	
	DEBUG(
		pc(mc); std::cout << "\tGenerating predecessor t=" << curTask << " m=" << vertex_to_method[curTo] << std::endl;
		pc(mc);
	   	if (curTask < htn->numActions) std::cout << "\t\tPRIM " << color(GREEN,htn->taskNames[curTask]) << " c=" << htn->actionCosts[curTask] << std::endl;
		else                           std::cout << "\t\tABST " << color(BLUE,htn->taskNames[curTask]) << " method " << color(CYAN, htn->methodNames[method]) << 
													" #" << method << std::endl;
		);

	///////////////////////////////////////// can we do this edge backwards?
	// state contains in v the current state and in v'' the stack push state of the successor
	if (curTask < htn->numActions || htn->numSubTasks[method] == 0) {
		// do a pre-image
	   	// I have no way of knowing how the push state has looked like, so set it to all possible ones
		BDD previousState = (curTask < htn->numActions) ?
			(trs[curTask].preimage(stateStack.front()).AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux)):
			(stateStack.front().AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux));

		if (previousState.IsZero()) {
			DEBUG(pc(mc); std::cout << color(RED,"\t\tRegression through the current state is impossible.") << std::endl);
			assert(trs[curTask].preimage(stateStack.front()).IsZero());
				
			return false;
		}
		DEBUG(pc(mc); std::cout << "\t\tRegressed through state." << std::endl);
		BDD secondStackState = stateStack.front().AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsPre); // remove v, push state is in v', but this is already 1

		// push the state onto the stack
		methodStack.push_front(curTo);
		taskStack.push_front(curTask);
		stateStack.pop_front();
		stateStack.push_front(secondStackState);
		stateStack.push_front(previousState);
	} else {
		if (htn->numSubTasks[method] == 1){
			if (taskStack[0] != htn->subTasks[method][0]){
				DEBUG(pc(mc); std:: cout << color(RED,"\tFirst task on stack does not match method.") << std::endl);
				return false;
			}

			// state does not change only the task
			taskStack.pop_front();		
			taskStack.push_front(curTask);
		} else if (htn->numSubTasks[method] == 2){ // something else cannot happen
			DEBUG(pc(mc); std::cout << "\tMethod expecting " << tasks_per_method[method].first << " " << tasks_per_method[method].second << " M " << vertex_to_method[curTo] << std::endl);
			if (taskStack.size() < 2) {
				DEBUG(pc(mc); std:: cout << color(RED,"\tStack not large enough.") << std::endl);
				return false;
			}
			if (taskStack[0] != tasks_per_method[method].first){
				DEBUG(pc(mc); std:: cout << color(RED,"\tFirst task on stack does not match method.") << std::endl);
				return false;
			}
		
			// check whether this method actually does to the vertex we are expecting
			if (taskStack[1] != tasks_per_method[method].second || curTo != methodStack[1]){
				DEBUG(pc(mc); 
					if (taskStack[1] != tasks_per_method[method].second)
						std::cout << color(RED,"\tSecond task on stack does not match method.") << std::endl;
					else 
						std::cout << color(RED,"\tCan't go to ") << vertex_to_method[curTo] << " on stack is " << vertex_to_method[methodStack[1]] << std::endl;
					);
				return false;
			}

			BDD previousState = stateStack.front() * biimp;
			if (previousState.IsZero()){
				DEBUG(pc(mc); std::cout << color(RED,"\t\tNo unpushable state exists.") << std::endl);
				return false;
			}
	
			previousState = previousState.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux); // get v <=> v'
			stateStack.pop_front();
			previousState *= stateStack.front();
			previousState = previousState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);

			if (previousState.IsZero()){
				DEBUG(pc(mc); std::cout << color(RED,"\t\tPush state of method does not fit.") << std::endl);
				return false;
			}
	
			stateStack.pop_front();
			stateStack.push_front(previousState);	
			taskStack.pop_front();
			taskStack.pop_front();
			taskStack.push_front(curTask);
			methodStack.pop_front();
		}
	}
	
	DEBUG(
		pc(mc); std::cout << "\t\tStack (after modification): " << taskStack.size() << std::endl;
		printStack(mc,taskStack,methodStack,1)
	);
	
	assert(taskStack.size() == methodStack.size());
	return true;
}

///////////////////////////////////////////////////////////////////7
reconstructed_plan extract2From(int curCost, int curDepth,
	int targetTask,
	int targetCost,
	BDD targetState,
	std::deque<int> taskStack,
	std::deque<int> methodStack,
	std::deque<BDD> stateStack,
	Model * htn,
	symbolic::SymVariables & sym_vars
		){
#ifndef NDEBUG
	int mc = debug_counter++;
#endif
	assert(taskStack.size() == methodStack.size());
	assert(taskStack.size() == stateStack.size());
	
	int taskStackHead = taskStack.front();
	int toStackHead = methodStack.front();


	DEBUG(pc(mc); std::cout << color(YELLOW,"New edge at") << " c=" << curCost << " d=" << curDepth << " with causing stack. Where was it new?" << std::endl);
	DEBUG(pc(mc); std::cout << "\t\ttarget " << targetTask << " target cost " << targetCost << std::endl);

	if (targetCost != -1 && curCost < targetCost){
		DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"too expensive") << std::endl);
		//exit(0);
		return get_fail();
	}

	if (curCost == 0 && curDepth == 1){
		if (!(targetState * stateStack.front()).IsZero()){
			DEBUG(pc(mc); std::cout << "\t\t" << color(GREEN,"Got to init. DONE!") << std::endl);
			return get_empty_success(stateStack.front() * targetState);
		} else {
			DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Got to init, but states does not fit") << std::endl);
			return get_fail();
		}
	}

	
	// look at my sources
	std::tuple<int,int> tup = {taskStackHead, toStackHead};
	auto & myPredecessors = (curDepth == 0) ? prim_q[curCost][tup] : abst_q[curCost][curDepth][tup];
	DEBUG(pc(mc); std::cout << "\tKnown causes: " << myPredecessors.size() << std::endl);
	if (myPredecessors.size() == 0){
		DEBUG(pc(mc); std::cout << "\t" << color(RED,"Stack has no known causes. This is not possible as we got here somehow.") << std::endl);
		exit(0);
	}
	std::map<std::pair<int,int>,std::vector<tracingInfo> > groupedTracesOrdinary;
	std::vector<tracingInfo> tracesEpsilon;
	for (auto & preItem : myPredecessors){
		auto & [preCost, preDepth, preTask, preTo, _method, cost, getHere, extraCost] = preItem;
		
		if (cost != -1 && getHere >= 0)
			tracesEpsilon.push_back(preItem);
		else
			groupedTracesOrdinary[std::make_pair(preCost + extraCost, preDepth)].push_back(preItem);
		
		
		DEBUG(pc(mc); std::cout << "\t\tEdge c=" << preCost << " d=" << preDepth << " t=" << preTask << " m=" << (preTo != -1?vertex_to_method[preTo]:-1) << " ec=" << cost << " ed=" << getHere << " epC=" << extraCost << std::endl;);
	}

	// try out the different options
	for (auto & [_pre, tracings] : groupedTracesOrdinary){
		auto & [preCost, preDepth] = _pre;
		DEBUG(pc(mc); std::cout << "\t" << color(YELLOW, "Current stack might have been new at") << " c=" << preCost << " d=" << preDepth << std::endl);

		
		std::deque<BDD> newStateStack = stateStack;
		
		// the state has to be in the edge where we think it originated
		ensureBDD(taskStack.front(), methodStack.front(), preCost, preDepth, sym_vars); // edge might be fully new
		auto [p2Cost, p2Depth] = prev(preCost, preDepth);
		ensureBDD(taskStack.front(), methodStack.front(), p2Cost, p2Depth, sym_vars); // edge might be fully new
		
		// stack head must be present in this round, but not in the previous one
		newStateStack[0] = newStateStack[0] *  edges[0][taskStack.front()][methodStack.front()][preCost][preDepth];
		newStateStack[0] = newStateStack[0] * !edges[0][taskStack.front()][methodStack.front()][p2Cost][p2Depth];
		if (newStateStack[0].IsZero()){
			DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Source state is not new") << std::endl;);
			continue;
		} else {
			DEBUG(pc(mc); std::cout << "\t\t" << color(GREEN,"Source state is new") << std::endl;);
		}



		
		///////////////////////////////////////////////////// check whether the stack exists at this time and if not repair it (internal edges might have been inserted into the stack)
		// check whether the stack is possible in total
		newStateStack = checkStack(
#ifndef NDEBUG
			mc,
#endif
			preCost,preDepth,taskStack,methodStack,newStateStack,htn,sym_vars);
		
		if (newStateStack.front().IsZero()){
			DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Source stack does not exist") << std::endl;);
			continue;
		}
		DEBUG(pc(mc); std::cout << color(GREEN,"\t\tValid stack state.") << std::endl);

		DEBUG(
				pc(mc); std::cout << color(GREEN,"POINT OF NO RETURN") << std::endl;
				pc(mc); std::cout << "I have a state and a stack that were newly inserted at" << std::endl;
				pc(mc); std::cout << "c=" << preCost << " d=" << preDepth << std::endl;
				printStack(mc,taskStack,methodStack,1);
				std::cout << "#####################################################################################" << std::endl;
				std::cout << "#####################################################################################" << std::endl;
		);
	
		std::cout << "CD " << curCost << " " << curDepth << std::endl;	
		if (curCost == 0 && curDepth == 1){
			if (!(targetState * newStateStack.front()).IsZero()){
				DEBUG(pc(mc); std::cout << "\t\t" << color(GREEN,"Got to init. DONE!") << std::endl);
				return get_empty_success(newStateStack.front() * targetState);
			} else {
				DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Got to init, but states does not fit") << std::endl);
				return get_fail();
			}
		}


		std::cout << "CC " << targetTask << " " << taskStack.size() << " " << taskStack[0] << " " << targetCost << " " << curCost << std::endl;
		if (targetTask != -1 && taskStack.size() == 1 && taskStack[0] == targetTask && targetCost == preCost){
			if (!(newStateStack.front() * targetState).IsZero()){
				DEBUG(pc(mc); std::cout << "\t\t" << color(GREEN,"Got to target task on stack.") << std::endl);
				return get_empty_success(newStateStack.front() * targetState);
			} else {
				DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Got to target task on stack, but state does not fit.") << std::endl);
				return get_fail();
			}
		}
		

#ifndef NDEBUG
		int roundCounter = 0;
#endif
		for (auto & [_1, _2, preTask, preTo, appliedMethod, cost, depth, extraCost] : tracings){
			DEBUG(roundCounter++);
			DEBUG(pc(mc); std::cout << color(YELLOW, "I could have been inserted by ") << " c=" << preCost - extraCost << " d=" << preDepth << 
					" t=" << preTask << " m=" << (preTo != -1 ? vertex_to_method[preTo] : -1) << " cost=" << cost << " depth=" << depth << " extraCost=" << extraCost << std::endl);
			assert(!(cost != -1 && depth >= 0));
		
			if (cost != -1) { // depth < 0, indicates additional cost edge
				int costCarriedByTheEdge = cost;
				
				DEBUG(pc(mc); std::cout << "\t" << color(YELLOW,"This edge carries additional cost.") << " cost=" << cost << " depth=" << depth << " delta=" << costCarriedByTheEdge << " m=" << appliedMethod << std::endl);
				assert(-depth-1 == toStackHead);
				assert(preTo == methods_with_two_tasks_vertex[appliedMethod]);

				int abstractTask = htn->decomposedTask[appliedMethod];

				DEBUG(pc(mc); std::cout << "\t\tPossible source states: " << edgesExtraCostSource[preTo][tasks_per_method[appliedMethod].second][toStackHead][cost].size() << std::endl);
				
				// construct the predecessor stack s.t. we can cut it in two
				std::deque<int> taskStackPredecessor = taskStack;
				std::deque<int> methodStackPredecessor = methodStack;
				std::deque<BDD> stateStackPredecessor = newStateStack;

				// see whether we can generate the predecessor and it is there
				bool predecessorsIsPossible = generatePredecessor(preTask,preTo,
						taskStackPredecessor,methodStackPredecessor,stateStackPredecessor,
						appliedMethod,htn,sym_vars);
				if (!predecessorsIsPossible) {
					DEBUG(pc(mc); std::cout << "\t" << color(RED,"Predecessor cannot account for the new state.") << std::endl;);
					continue;
				}


				BDD costCheckBDD = edgesExtraCost[preTo][tasks_per_method[appliedMethod].second][toStackHead][extraCost];
				
				stateStackPredecessor[0] *= costCheckBDD.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux).SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);
				stateStackPredecessor[1] *= costCheckBDD; 
				if (stateStackPredecessor[0].IsZero() || stateStackPredecessor[1].IsZero()){
					DEBUG(pc(mc); std::cout << "\t" << color(RED,"Predecessor cannot account for the new state.") << std::endl;);
					continue;
				}

				
				auto [p2Cost, p2Depth] = prev(preCost - extraCost, preDepth);

				// ensure that this stack is actually present in the previous time step
				stateStackPredecessor = checkStack(
#ifndef NDEBUG
					mc,
#endif
					p2Cost,p2Depth,taskStackPredecessor,methodStackPredecessor,stateStackPredecessor,htn,sym_vars);
		
				if (stateStackPredecessor.front().IsZero()){
					DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Source stack does not exist") << std::endl;);
					continue;
				}
				
				
				DEBUG(pc(mc); std::cout << color(GREEN,"\t\tValid stack state.") << std::endl);

				DEBUG(
						pc(mc); std::cout << color(GREEN,"POINT OF NO RETURN") << std::endl;
						pc(mc); std::cout << "Splitting at time" << std::endl;
						pc(mc); std::cout << "c=" << preCost - extraCost<< " d=" << preDepth << std::endl;
						printStack(mc,taskStackPredecessor,methodStackPredecessor,1);
						pc(mc); std::cout << "Number of extra edge cost sources: " << edgesExtraCostSource[preTo][tasks_per_method[appliedMethod].second][toStackHead][cost].size() << std::endl;
						std::cout << "#####################################################################################" << std::endl;
						std::cout << "#####################################################################################" << std::endl;
				);

				
				// iterate over all possible source states
				for (auto & [sourceCost,sourceDepth,continueCost,continueDepth] : edgesExtraCostSource[preTo][tasks_per_method[appliedMethod].second][toStackHead][cost]){
					DEBUG(pc(mc); std::cout << "\t\t" << color(YELLOW,"A possible source is") << " c=" << sourceCost << " d=" << sourceDepth << " cc=" << continueCost << " cd=" << continueDepth << std::endl);
					
					
					DEBUG(pc(mc); std::cout << "Cost check " << costCarriedByTheEdge << ", " << curCost << ", " << preCost << ", " << continueCost << ", " << extraCost << " - " << sourceCost << " - " << htn->actionCosts[preTask] << std::endl);
					//if (costCarriedByTheEdge != preCost - continueCost - htn->actionCosts[preTask]) continue;
					//int additionCost = preCost - cont;
					//if (additionCost < 0) continue;

					//DEBUG(pc(mc); std::cout << "\t\t\tAdditional cost " << additionCost << std::endl);
					//DEBUG(pc(mc); std::cout << "\t\t\tEdge is 0 " << tasks_per_method[appliedMethod].first << " " << vertex_to_method[preTo] << std::endl);
					
					// split the stack
					std::deque<int> taskStackFirstPart; taskStackFirstPart.push_front(taskStackPredecessor.front());
					std::deque<int> methodStackFirstPart; methodStackFirstPart.push_front(methodStackPredecessor.front());
					std::deque<BDD> stateStackFirstPart; stateStackFirstPart.push_front(stateStackPredecessor.front());
					
					// the state that we started on, but only the ones
					auto [sourceCostPrev, sourceDepthPrev] = prev(sourceCost, sourceDepth);
					ensureBDD(tasks_per_method[appliedMethod].first,preTo,sourceCost,sourceDepth,sym_vars);
					ensureBDD(tasks_per_method[appliedMethod].first,preTo,sourceCostPrev,sourceDepthPrev,sym_vars);
					BDD sourceState = edges[0][tasks_per_method[appliedMethod].first][preTo][sourceCost][sourceDepth];
					BDD sourceStatePrev = edges[0][tasks_per_method[appliedMethod].first][preTo][sourceCostPrev][sourceDepthPrev];
					BDD newSourceState = sourceState * !sourceStatePrev;

					assert(!sourceState.IsZero());
					assert(!newSourceState.IsZero());
				
					// now we are extracting until we have found the first task of the method on the edge
					int firstTask = tasks_per_method[appliedMethod].first;
					
					reconstructed_plan a = extract2From(preCost - extraCost, preDepth, firstTask, sourceCost, newSourceState, 
							taskStackFirstPart, methodStackFirstPart, stateStackFirstPart, htn, sym_vars );

					if (!a.success){
						DEBUG(pc(mc); std::cout << "Extraction of the first part failed. This cannot happen." << std::endl;);
						//exit(0);
						//return get_fail();
						continue;
					}
					
					int prefixExpectedCost = preCost - extraCost - sourceCost - htn->actionCosts[preTask];

					if (a.primitiveCost(htn) != prefixExpectedCost){
						DEBUG(pc(mc));
						std::cout << "Cost I was expecting " << prefixExpectedCost << " (=" << preCost - extraCost << "-" << sourceCost << ") but got " << a.primitiveCost(htn) << std::endl;
						exit(0);
					}

					// after the additional cost path has been explored, the head of the stack is now the head of the cost path when it finished
					stateStackPredecessor[0] = a.endState;
					taskStackPredecessor[0] = firstTask; 

					// ensure that this stack is actually present in the previous time step
					stateStackPredecessor = checkStack(
#ifndef NDEBUG
						mc,
#endif
						continueCost,continueDepth,taskStackPredecessor,methodStackPredecessor,stateStackPredecessor,htn,sym_vars);
		
					if (stateStackPredecessor.front().IsZero()){
						DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Second source stack does not exist") << std::endl;);
						continue;
					}
					DEBUG(pc(mc); std::cout << "\t\t" << color(GREEN,"Second part of the stack exists") << std::endl;);

					// apply the method there backwards
					predecessorsIsPossible = generatePredecessor(abstractTask,methodStackPredecessor[1],
							taskStackPredecessor,methodStackPredecessor,stateStackPredecessor,
							appliedMethod,htn,sym_vars);
					if (!predecessorsIsPossible) {
						DEBUG(pc(mc); std::cout << "\t" << color(RED,"Predecessor cannot account for the new state.") << std::endl;);
						continue;
					}

					DEBUG(pc(mc); std::cout << "\t\t" << color(MAGENTA,"Second part continuation ") << " cc=" << continueCost << " cd=" << continueDepth << std::endl);
					reconstructed_plan b = extract2From(continueCost, continueDepth, targetTask, targetCost, targetState,
							taskStackPredecessor, methodStackPredecessor, stateStackPredecessor, htn, sym_vars);
					
					if (!b.success){
						DEBUG(pc(mc); std::cout << "Extraction of the second part failed. This cannot happen." << std::endl;);
						//exit(0);
						//return get_fail();	
						continue;
					}
					DEBUG(pc(mc); std::cout << color(GREEN,"Extraction of the second part successful.") << std::endl;);
					
					a = addToPlan(a,preTask, appliedMethod, htn);
					
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
					//int aRoot = a.root;
					
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

				DEBUG(pc(mc); std::cout << "Backtracking beyond decision to take cost edge. This cannot be wrong as the causing stack existed." << std::endl;);
				//exit(0);
				//return get_fail();
				continue;
			} else {
				DEBUG(pc(mc); std::cout << "\t==> regular operation" << std::endl;);
				std::deque<int> taskStackPredecessor = taskStack;
				std::deque<int> methodStackPredecessor = methodStack;
				std::deque<BDD> stateStackPredecessor = newStateStack;

				// see whether we can generate the predecessor and it is there
				bool predecessorsIsPossible = generatePredecessor(preTask,preTo,
						taskStackPredecessor,methodStackPredecessor,stateStackPredecessor,
						appliedMethod,htn,sym_vars);

				if (!predecessorsIsPossible) {
					DEBUG(pc(mc); std::cout << "\t" << color(RED,"Predecessor cannot account for the new state.") << std::endl;);
					continue;
				} else {
					DEBUG(pc(mc); std::cout << "\t" << color(GREEN,"Predecessor can account for the new state.") << std::endl;);
				}

				std::deque<BDD> cS = checkStack(
#ifndef NDEBUG
			mc,
#endif
			preCost,preDepth,taskStackPredecessor,methodStackPredecessor,stateStackPredecessor,htn,sym_vars);

				if (cS[0].IsZero()){
					DEBUG(pc(mc); std::cout << "\t" << color(RED,"Non existing starting state ...") << std::endl;);
				}

				{	
		std::deque<BDD> newStateStack = stateStack;
		
		// the state has to be in the edge where we think it originated
		ensureBDD(taskStack.front(), methodStack.front(), preCost, preDepth, sym_vars); // edge might be fully new
		auto [p2Cost, p2Depth] = prev(preCost, preDepth);
		ensureBDD(taskStack.front(), methodStack.front(), p2Cost, p2Depth, sym_vars); // edge might be fully new
		
		// stack head must be present in this round, but not in the previous one
		newStateStack[0] = newStateStack[0] *  edges[0][taskStack.front()][methodStack.front()][preCost][preDepth];
		newStateStack[0] = newStateStack[0] * !edges[0][taskStack.front()][methodStack.front()][p2Cost][p2Depth];
		if (newStateStack[0].IsZero()){
			DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"Old state is not new ") << preCost << " " << preDepth << "/" << p2Cost << " " << p2Depth << std::endl;);
			continue;
		} else {
			DEBUG(pc(mc); std::cout << "\t\t" << color(GREEN,"Old state is new between ") << preCost << " " << preDepth << "/" << p2Cost << " " << p2Depth << std::endl;);
		}
				}

					{	
		std::deque<BDD> newStateStack = stateStackPredecessor;
		
		// the state has to be in the edge where we think it originated
		ensureBDD(taskStackPredecessor.front(), methodStackPredecessor.front(), preCost, preDepth, sym_vars); // edge might be fully new
		auto [p2Cost, p2Depth] = prev(preCost, preDepth);
		ensureBDD(taskStackPredecessor.front(), methodStackPredecessor
				.front(), p2Cost, p2Depth, sym_vars); // edge might be fully new
		
		// stack head must be present in this round, but not in the previous one
		newStateStack[0] = newStateStack[0] *  edges[0][taskStackPredecessor.front()][methodStackPredecessor.front()][preCost][preDepth];
		newStateStack[0] = newStateStack[0] * !edges[0][taskStackPredecessor.front()][methodStackPredecessor.front()][p2Cost][p2Depth];
		if (newStateStack[0].IsZero()){
			DEBUG(pc(mc); std::cout << "\t\t" << color(RED,"New state is not new ") << preCost << " " << preDepth << "/" << p2Cost << " " << p2Depth << std::endl;);
			//continue;
		} else {
			DEBUG(pc(mc); std::cout << "\t\t" << color(GREEN,"New state is new between ") << preCost << " " << preDepth << "/" << p2Cost << " " << p2Depth << std::endl;);
		}
				}

	


				reconstructed_plan a = extract2From(preCost, preDepth, targetTask, targetCost, targetState,
						taskStackPredecessor, methodStackPredecessor, stateStackPredecessor, htn, sym_vars);
				
				if (!a.success){
					DEBUG(pc(mc);std::cout << "\t" << color(RED,"Not new in previous round. Trying the next possibility. ("  + to_string(roundCounter) + "/" + to_string(tracings.size()) + ")") << std::endl;);
					continue;
				}

				a = addToPlan(a,preTask, appliedMethod, htn);

				if (a.primitiveCost(htn) != preCost - targetCost){
					DEBUG(std::cout << a.primitiveCost(htn) << " " << preCost << " " << targetCost << std::endl);
					std::cout << color(RED,"Cost does not fit") << std::endl;
				}
				assert(a.primitiveCost(htn) == preCost - targetCost);
				return a;
			}
		}
		
		
		// backtracking is not allowed to fail, since we know that getting to this point is possible with the new state
		DEBUG(pc(mc); std::cout << color(RED,"Backtracking failed at this point ... ") << std::endl);
		//exit(0);
	}




	for (auto & [preCost, preDepth, preTask, preTo, appliedMethod, cost, depth, extraCost] : tracesEpsilon){
		DEBUG(pc(mc); std::cout << color(YELLOW, "\tI could have been inserted by ") << " c=" << preCost << " d=" << preDepth << 
				" t=" << preTask << " m=" << (preTo != -1 ? vertex_to_method[preTo] : -1) << std::endl);
		
		assert(cost != -1 && depth >= 0);
	
		////////////////////////////////////////////////////////////////////////////////////////////////	
		// inserted by epsilon application
		////////////////////////////////////////////////////////////////////////////////////////////////	
		DEBUG(pc(mc); std::cout << "\t" << color(YELLOW,"This edge was inserted due to an epsilon application.") << " cost=" << cost << " depth=" << depth << " delta=" << extraCost << " m=" << appliedMethod << std::endl);


		int toVertex = methods_with_two_tasks_vertex[appliedMethod];
		assert(preTask == htn->decomposedTask[appliedMethod]);
		
		DEBUG(pc(mc); std::cout << "\t\tTask on the main edge is " << htn->taskNames[preTask] << " (#" << preTask << ")" << std::endl);
		DEBUG(pc(mc); std::cout << "\t\t\tOptions: " << eps_inserted[toVertex][cost][depth].size() << std::endl);

		/// eps contains in v the state after the epsilon application
		// and in v' the state in which the base task was pushed
		BDD edgeBDD = eps[toVertex][cost][depth];
		auto [pCost, pDepth] = prev(cost, depth);
		edgeBDD = edgeBDD * !eps[toVertex][pCost][pDepth]; // only the new epsilon states are interesting
		// and of the new ones only the ones that will lead to the current stack head
		edgeBDD *= stateStack.front(); // this has now v (current state) v' (stack push state) v'' (stack continuation)
		DEBUG(pc(mc); std::cout << "\t\t\tEdgeBDD is " << cost << " " << depth << " / " << pCost << " " << pDepth << std::endl);
		
		if (edgeBDD.IsZero()){
			DEBUG(pc(mc); std::cout << "\t\t\t" << color(RED,"Epsilon could not have caused current stack head.") << std::endl;);
			continue;
		}

		// how would the stack have looked like before this operation?
		std::deque<int> taskStackPredecessor = taskStack;
		std::deque<int> methodStackPredecessor = methodStack;
		std::deque<BDD> stateStackPredecessor = stateStack;
		// add the edge that was replaced by epsilon
		int firstTask = tasks_per_method[appliedMethod].first;
		taskStackPredecessor.push_front(firstTask);
		methodStackPredecessor.push_front(toVertex);
		stateStackPredecessor.pop_front();
		stateStackPredecessor.push_front(edgeBDD.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsPre));
		stateStackPredecessor.push_front(
				edgeBDD.
					AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux).
					AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsPre).
					SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux) * biimp
					);
		
		
		//printStack(mc,taskStackPredecessor,methodStackPredecessor,1);
	
		bool predecessorsIsPossible = generatePredecessor(preTask,preTo,
				taskStackPredecessor,methodStackPredecessor,stateStackPredecessor,
				appliedMethod,htn,sym_vars);
		if (!predecessorsIsPossible) {
			DEBUG(pc(mc); std::cout << "\t" << color(RED,"Predecessor cannot account for the new state.") << std::endl;);
			continue;
		}

		DEBUG(pc(mc); std::cout << color(GREEN,"\t\tValid stack state.") << std::endl);

		DEBUG(
				pc(mc); std::cout << color(GREEN,"POINT OF NO RETURN") << std::endl;
				pc(mc); std::cout << "Edge Inserted based on epsilon progression" << std::endl;
				pc(mc); std::cout << "c=" << preCost << " d=" << preDepth << std::endl;
				pc(mc); std::cout << "Number of options: " << eps_inserted[toVertex][cost][depth].size() << std::endl;
				std::cout << "#####################################################################################" << std::endl;
				std::cout << "#####################################################################################" << std::endl;
		);


		for (auto & [_preCost2, _preDepth2, lastTask, _toAgain, method2, remainingTask, remainingMethod, _] : eps_inserted[toVertex][cost][depth]){
			DEBUG(pc(mc); std::cout << color(YELLOW,"Vertex was reachable with Epsilon when ") << " next on stack: t=" << remainingTask << " m=" << remainingMethod << " previous task: " << lastTask << " method: " << method2 << " " << endl);
		

			//////////////////////////////////////////////// Reconstruct a plan for the epsilon transition
			// start reconstruction with empty stack	
			std::deque<int> epsilonTaskStack;
			std::deque<int> epsilonMethodStack;
			std::deque<BDD> epsilonStateStack;

			epsilonTaskStack.push_back(remainingTask);
			epsilonMethodStack.push_back(remainingMethod);
			ensureBDD(remainingTask, remainingMethod, pCost, pDepth, sym_vars);
			epsilonStateStack.push_back(edges[0][remainingTask][remainingMethod][cost][depth] * !edges[0][remainingTask][remainingMethod][pCost][pDepth]);
		
			// regress through the last operation that we performed
			predecessorsIsPossible = generatePredecessor(lastTask,toVertex,
				epsilonTaskStack,epsilonMethodStack,epsilonStateStack,
				method2,htn,sym_vars);

			if (!predecessorsIsPossible) {
				DEBUG(pc(mc); std::cout << "\t" << color(RED,"Predecessor cannot account for the new state.") << std::endl;);
				continue;
			}

			epsilonTaskStack.pop_back();
			epsilonMethodStack.pop_back();
			epsilonStateStack.pop_back();

			// only a target state if push and current are identical	
			BDD nextTargetState = edgeBDD.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsPre).
				AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux).
				SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff) * biimp;
			
			reconstructed_plan a = extract2From(cost, depth, firstTask, cost - extraCost, nextTargetState, 
					epsilonTaskStack, epsilonMethodStack, epsilonStateStack, htn, sym_vars);
		
			if (!a.success){
				DEBUG(pc(mc); std::cout << "Extraction of the epsilon reduction failed. This cannot happen." << std::endl;);
				//exit(0);
				//return get_fail();	
				continue;
			}
			
			/*if (a.primitiveCost(htn) != extraCost){
				DEBUG(pc(mc)); std::cout << "Epsilon: I was expecting " << extraCost << " but got " << a.primitiveCost(htn) << std::endl;
				exit(0);	
			}*/
			DEBUG(pc(mc); std::cout << color(GREEN,"Extraction of the epsilon reduction succeeded.") << std::endl;);
			
			//DEBUG(pc(mc)); std::cout << "\t\t\tEpsilon: I got the correct cost " << a.primitiveCost(htn) << " going on " << preCost << " " << preDepth << " " << preTask << " " << sm.front() << std::endl;
		
			// clean re-initialisation of stacks
			stateStackPredecessor[0] *= a.endState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
		
			if (stateStackPredecessor[0].IsZero()) {
				DEBUG(pc(mc); std::cout << "\t" << color(RED,"Predecessor cannot account for the new state.") << std::endl;);
				continue;
			}
		
			
			DEBUG(
				pc(mc); std::cout << "Extracting remaining plan from c=" << preCost << " d=" << preDepth << std::endl;
				printStack(mc,taskStackPredecessor,methodStackPredecessor,1);
			);




			reconstructed_plan b = extract2From(preCost, preDepth, targetTask, targetCost, targetState,
					taskStackPredecessor, methodStackPredecessor, stateStackPredecessor, 
					htn, sym_vars);
		
			if (!b.success) {
				DEBUG(pc(mc); std::cout << "Extraction of epsilon remainder failed. This cannot happen." << std::endl;);
				//exit(0);
				//return get_fail();	
				continue;
			}
			
			
			a = addToPlan(a, lastTask, method2, htn);
			b = addToPlan(b, preTask, appliedMethod, htn);

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
			int bStackTop = b.currentStack.front();
			b.currentStack.pop_front();
			// replace 
			for (auto & x : a.primitive_plan)
				if (get<0>(x) == aRoot)
					get<0>(x) = bStackTop;
			for (auto & x : a.abstract_plan)
				if (get<0>(x) == aRoot)
					get<0>(x) = bStackTop;
			
			
			a.currentStack = b.currentStack;
			a.root = b.root;
			
			//a.printPlan(htn);

			return a;
		}

		DEBUG(pc(mc); std::cout << color(RED,"Backtracking failed at this point ... ") << std::endl);
		//exit(0);
	}


	// what to do? Nothing is new here
	return get_fail();
}





void extract(int curCost, int curDepth, int curTask, int curTo,
	BDD state,
	int method, 
	Model * htn,
	symbolic::SymVariables & sym_vars
	){
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << "========================================================================" << std::endl;


	state = state.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff); // remove the effect variables
	state = state.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux); // remove the auxiliary variables
	
	BDD init = sym_vars.getStateBDD(htn->s0List, htn->s0Size);

	/////////////////////////////////////////////////////////////////////////////////
	// compute the new part of the edge that led us to the goal
	BDD previousState = state;
	if (curTask < htn->numActions)
		previousState = trs[curTask].preimage(state); 
	previousState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);

	// construct the stacks
	std::deque<int> st; st.push_back(curTask);
	std::deque<int> sm; sm.push_back(curTo);
	std::deque<BDD> ss; ss.push_back(previousState);
	
	std::clock_t plan_start = std::clock();
	reconstructed_plan a = extract2From(curCost, curDepth, htn->initialTask, 0, init, st, sm, ss, htn, sym_vars);

	int stackTask = a.currentStack.front(); a.currentStack.pop_front();
	// add a primitive action
	a.primitive_plan.push_back({stackTask,curTask});
	
	
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


void print_states_in_bdd(symbolic::SymVariables & sym_vars, Model * htn, BDD & states){
	return;
	BDD firstStates = states.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff).AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
	
	for (vector<int> state : sym_vars.getStatesFrom(firstStates)){
        std::cout << color(BLUE,"State:") << endl;
        for (int val : state){
                //if (count[val] == states.size()) continue;
                //std::cout << "\t" << val << "/" << htn->numStateBits << endl;
                std::cout << "\t" << htn->factStrs[val] << endl;
        }
        std::cout << std::endl << std::endl;
    }
    std::cout << color(BLUE,"LAST State") << endl;
}

//================================================== planning algorithm
//#define NDEBUG
//#define DEBUG(x)


void build_automaton(Model * htn){
	DEBUG(std::cout << "Running automaton building in debug mode" << std::endl);
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

	// initialise the bi-implication
	biimp = sym_vars.oneBDD();
	for (int i = 0; i < htn->numVars; i++)
		biimp *= sym_vars.auxBiimp(i); // v_i = v_i''



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
	eps_inserted.resize(number_of_vertices);

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

	std::map<int,std::map<int,BDD>> _empty_eps;
	_empty_eps[0][0] = sym_vars.zeroBDD();
	for (int i = 0; i < number_of_vertices; i++) eps.push_back(_empty_eps);

#define put push_back
//#define put insert

	std::tuple<int,int> _tup = {htn->initialTask, 1};
	tracingInfo _from = {0,0,htn->initialTask,1,-1,-1,-1,-1};
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
				//DEBUG(std::cout << "Case A" << std::endl);
				DEBUG(insertCost = currentCost; insertDepth = currentDepthInAbstract + 1);
			} else {
				insertQueue = &(abst_q[currentCost + extraCost][1][tup]);
				//DEBUG(std::cout << "Case B" << std::endl);
				DEBUG(insertCost = currentCost + extraCost; insertDepth = 1);
			}
		} else {
			// primitive with cost
			insertQueue = &(prim_q[currentCost + extraCost + htn->actionCosts[task]][tup]);
			//DEBUG(std::cout << "Case C" << std::endl);
			DEBUG(insertCost = currentCost + extraCost + htn->actionCosts[task]; insertDepth = 0);
		}

		if (!insertOnlyIfNonEmpty || insertQueue->size() || (task < htn->numActions && htn->actionCosts[task] != 0)){
			DEBUG(std::cout << "\t\t\t\t\t\t" << color(GREEN,"Inserting") << " " << task << " " << vertex_to_method[to] << " to " << insertCost << " " << insertDepth << std::endl);
			DEBUG(std::cout << "\t\t\t\t\t\t" << color(GREEN," tracking") << " " << currentCost << " " << currentDepthInAbstract << " " << fromTask << " " << fromTo << " " << method << " c=" << cost << " " << extraCost << std::endl);
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
			int task       = get<0>(entry.first);
			int to         = get<1>(entry.first);
			
			unordered_set<int> handledCosts;
			for(tracingInfo & info : entry.second){
				//int sourceCost = get<0>(info) + get<7>(info);
				int sourceCost = get<0>(info);
				if (get<7>(info) != get<5>(info)) sourceCost += get<7>(info);
				if (handledCosts.count(sourceCost)) continue;
				handledCosts.insert(sourceCost);

	#ifndef NDEBUG
				if (step % 1 == 0){
	#else
				if (step % 10000 == 0){
	#endif
					std::cout << color(BLUE,"STEP") << " #" << step << ": " << task << " " << vertex_to_method[to];
		   			std::cout << "\t\tTask: " << htn->taskNames[task] << std::endl;		
				}
				
				if (entry.second.size() == 0) {
					DEBUG(std::cout << "Ignoring " << task << " " << vertex_to_method[to] << std::endl);
					continue;
				}
			
				// determine which time step to consult for propagation
				if (currentDepthInAbstract != 0){
					lastCost = currentCost;
					lastDepth = currentDepthInAbstract - 1;
				} else {
					lastCost  = sourceCost;
					lastDepth = edges[0][task][to][sourceCost].rbegin()->first;
				}
	
				DEBUG(cout << "Last Cost " << lastCost << " Last Depth " << lastDepth << " Current Cost " << currentCost << " Current Depth " << currentDepthInAbstract << endl);
	
	
				BDD state = edges[0][task][to][lastCost][lastDepth];
				
				
				//sym_vars.bdd_to_dot(state, "state" + std::to_string(step) + ".dot");
		  
	
				
				if (state == sym_vars.zeroBDD()) {
					std::cout << color(RED,"state attached to edge is empty ... bug!") << std::endl;
					exit(0);
					continue; // impossible state, don't treat it
				}
	
				if (task < htn->numActions || htn->emptyMethod[task] != -1){
					BDD nextState = state;
					
					DEBUG(cout << "Current state" << endl;
					print_states_in_bdd(sym_vars,htn,nextState));
					
					if (task < htn->numActions){
						DEBUG(std::cout << "Prim: " << htn->taskNames[task] << std::endl);
						// apply action to state
						nextState = trs[task].image(state);
					} else {
						DEBUG(std::cout << "Empty Method: " << htn->taskNames[task] << std::endl);
					}
					
					nextState = nextState.SwapVariables(sym_vars.swapVarsEff, sym_vars.swapVarsAux);
					DEBUG(cout << "Image state" << endl;
					print_states_in_bdd(sym_vars,htn,nextState));
					
					
					if (nextState.IsZero()){
						DEBUG(std::cout << "\t\t" << color(RED," action is not applicable") << std::endl);
					} else {
						// check if already added
						// TODO: there is a bit faster code here
	 					BDD disjunct = eps[to][currentCost][currentDepthInAbstract] + nextState;  
						if (disjunct != eps[to][currentCost][currentDepthInAbstract]){
							eps[to][currentCost][currentDepthInAbstract] = disjunct;
							DEBUG(std::cout << "\t\t\t\t\t\t" << color(MAGENTA,"Adding to eps of ") << vertex_to_method[to] << std::endl);
							
	
							for (auto & [task2,tos] : edges[to]){
								for (auto & [to2,bdds] : tos){
									if (!bdds.count(lastCost) || !bdds[lastCost].count(lastDepth)) continue;
									DEBUG(std::cout << "\tContinuing edge: " << task2 << " " << vertex_to_method[to2] << "\t" << htn->taskNames[task2] << std::endl);
									
									BDD bdd = bdds[lastCost][lastDepth];
									BDD addStateWithIntermediate = nextState * bdd;
									BDD originalAddStateWithIntermediate = addStateWithIntermediate;
									// we only add something if it is not already at the edge ...
									ensureBDD(task2, to2, currentCost, currentDepthInAbstract, sym_vars);
									addStateWithIntermediate = addStateWithIntermediate * !edges[0][task2][to2][currentCost][currentDepthInAbstract];
									
									if (addStateWithIntermediate.IsZero()) continue; 
								   	
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
									
									BDD addState = addStateWithIntermediate.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);
								
									
	
	
									BDD edgeDisjunct = edges[0][task2][to2][currentCost][currentDepthInAbstract] + addState;
									if (edgeDisjunct == edges[0][task2][to2][currentCost][currentDepthInAbstract])
										continue;
									
									DEBUG(
									std::cout << "new states" << endl;
									print_states_in_bdd(sym_vars,htn,addState);
									std::cout << "current states" << endl;
									print_states_in_bdd(sym_vars,htn,edges[0][task2][to2][currentCost][currentDepthInAbstract]);
									std::cout << "all states" << endl;
									print_states_in_bdd(sym_vars,htn,edgeDisjunct));
									
	
									edges[0][task2][to2][currentCost][currentDepthInAbstract] = edgeDisjunct;
									DEBUG(if (task < htn->numActions) std::cout << "\tPrim: "; else std::cout << "\tMethod: ";
											std::cout << task2 << " " << vertex_to_method[to2] << std::endl);
									addQ(task2, to2, 0, task, to, htn->emptyMethod[task], -1, -1, false); // no method as primitive
							
									
									tracingInfo tracingInf = {currentCost, currentDepthInAbstract, task, to, htn->emptyMethod[task], task2, to2, -1};
									eps_inserted[to][currentCost][currentDepthInAbstract].push_back(tracingInf);
								
									
									//if (step == 141){
									//	exit(0);
									//}	
								}
							}
	
	
							if (to == 1){
							   	if (nextState * goal != sym_vars.zeroBDD()){
									std::cout << "Goal reached! Length=" << currentCost << " steps=" << step << std::endl;
									
									std::clock_t search_end = std::clock();
									double search_time_in_ms = 1000.0 * (search_end-preparation_end) / CLOCKS_PER_SEC;
									std::cout << "Search time: " << fixed << search_time_in_ms << "ms " << fixed << search_time_in_ms / 1000 << "s " << fixed << search_time_in_ms / 60000 << "min" << std::endl << std::endl;
									
									extract(currentCost, currentDepthInAbstract, task, to, nextState * goal, htn->emptyMethod[task], htn, sym_vars);
									return;
								} else {
									std::cout << "Goal not reached!" << std::endl;
								}
							}
						} else {
							DEBUG(std::cout << "\t\t" << color(RED," ... state is not new") << std::endl);
						}
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
							DEBUG(std::cout << "\t\t\t" << htn->taskNames[tasks_per_method[method].first] << " " << htn->taskNames[tasks_per_method[method].second] << std::endl);
						
							// This is what we would add to the internal edge
							// we have the current state in v' and the stack state in v''
							BDD r_temp = state.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff);
						
							// whether we take this or the last layer is not relevant, as we are the only ones in this layer that actually add something to the edge
							// remember: you have to have the same target node and the same task, i.e. be the same pair in the queue ....
							ensureBDD(methods_with_two_tasks_vertex[method], tasks_per_method[method].second, to, currentCost, currentDepthInAbstract, sym_vars);
							BDD actually_new_r_temp = r_temp * !edges[methods_with_two_tasks_vertex[method]][tasks_per_method[method].second][to][currentCost][currentDepthInAbstract];
	
							// new state for edge to method vertex
	
							BDD ss = state.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
							ss *= biimp;
	
							// we have to distinguish between the states that we already know and the ones that are new
							
							// determine the already known part of the edge
							ensureBDD(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], lastCost, lastDepth, sym_vars);
							BDD previousBDD = edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][lastCost][lastDepth];
							BDD alreadyKnown = ss * previousBDD;
	
							if (!alreadyKnown.IsZero()){
								DEBUG(std::cout << "\t\tPart of the state is already known." << std::endl);
								// handle the parts of the state by the first cost when the main edge was inserted first
	
								for (int getHere = 0; getHere <= lastCost; getHere++){
									auto lastOfCost = edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][getHere].end();
									if (lastOfCost == edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][getHere].begin())
										continue; // edge not there yet
										
									int lastDepth = (--lastOfCost)->first;
									if (getHere == currentCost)
										lastDepth  = currentDepthInAbstract;
	
									for (int getHereDepth = 0; getHereDepth <= lastDepth; getHereDepth++){
										ensureBDD(0,tasks_per_method[method].first,methods_with_two_tasks_vertex[method],getHere,getHereDepth ,sym_vars);
										BDD intersectionBDD = alreadyKnown * edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][getHere][getHereDepth];
	
										if (intersectionBDD.IsZero()) continue;
	
										DEBUG(std::cout << "\t\t\t" << color(YELLOW,"States first occurring") << " c=" << getHere << " d=" << getHereDepth << std::endl);
										DEBUG(
											if (intersectionBDD == alreadyKnown)
												std::cout << "\t\t\t" << "- these are all states" << std::endl;
											else
												std::cout << "\t\t\t" << "- these are not all states" << std::endl;
												);
	
	
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
												//if (cost == currentCost && depth == currentDepthInAbstract) break; // don't take epsilons from current layer, they will be handled elsewhere ...?
												
												// there is no reason to consider this, as there was no newly inserted epsilon state
												if (eps_inserted[methods_with_two_tasks_vertex[method]][cost][depth].size() == 0) continue; 
		
												// state after eps in v and stack push state in v'
												BDD epsCurrent = eps[methods_with_two_tasks_vertex[method]][cost][depth];
			
												// determine the state intersection, i.e. whether we actually have a possible epsilon shortcut at this state
												BDD epsilonIntersect = epsCurrent * sIntersect;
	
												if (epsilonIntersect.IsZero()) continue;
												DEBUG(std::cout << "\t\t\t\t" << color(YELLOW,"Can be converted to epsilon at ") << cost << " " << depth << std::endl);
												//DEBUG(
												//	if (epsilonIntersect == sIntersect)
												//		std::cout << "\t\t\t\t" << "- these are all states" << std::endl;
												//	else
												//		std::cout << "\t\t\t\t" << "- these are not all states" << std::endl;
												//		);
											
												////////////////////////////////////////////////////////////////	
												// found epsilon transition
												int actualCost = cost - getHere;
												int targetCost = currentCost + actualCost;
											
												// determine where this edge will go to	
												int targetDepth = 0;
												if (actualCost == 0)
													targetDepth = currentDepthInAbstract;
												
												DEBUG(std::cout << "\t\t\t\tEpsilon with cost " << actualCost << " from " << getHere << " target cost " << targetCost << " target depth " << targetDepth << std::endl);
												
												ensureBDD(tasks_per_method[method].second, to, targetCost, targetDepth, sym_vars); // add as an edge to the future
											
												// remove the v''
												BDD newState = epsilonIntersect.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
												newState = newState * actually_new_r_temp;
												// remove the push state, it is not relevant any more as we have passed it.
												newState = newState.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff);
												
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

										sIntersect = intersectionBDD.SwapVariables(sym_vars.swapVarsPre, sym_vars.swapVarsEff);
										
										// case 2: unfinished transitions
										// add this to the internal edge ... we will go through it at some point
										if (!sIntersect.IsZero()){
											DEBUG(std::cout << "\t\t\t\t" << color(RED,"intersection remaining that is not yet progressed to epsilon.") << std::endl);
											
											// reduce the intersect to only the state in v'
											sIntersect = sIntersect.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsAux);
											assert(sIntersect.AndAbstract(sym_vars.oneBDD(), sym_vars.existsVarsEff).IsOne());
										
											// get the new parts on the internal edge, they carry additional cost, as they are solely explored via the path from (getHere,getHereDepth)
											sIntersect = sIntersect * actually_new_r_temp;
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
														std::make_tuple(getHere, getHereDepth, currentCost, currentDepthInAbstract));
											}
										}
		
	
										alreadyKnown = alreadyKnown * !intersectionBDD;
										if (alreadyKnown.IsZero()){
											DEBUG(std::cout << "\t\t\t" << color(RED,"no state remaining, ending.") << std::endl);
											break;
										}
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
							ensureBDD(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], currentCost, currentDepthInAbstract, sym_vars);
							BDD currentBDD = edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][currentCost][currentDepthInAbstract];
							BDD disjunct2 = currentBDD + ss;
						   	if (disjunct2 != currentBDD){
							   edges[0][tasks_per_method[method].first][methods_with_two_tasks_vertex[method]][currentCost][currentDepthInAbstract] = disjunct2;
								
							   DEBUG(std::cout << "\t\t" << color(GREEN,"2 normal: ") << tasks_per_method[method].first << " " << vertex_to_method[methods_with_two_tasks_vertex[method]] << std::endl);
							   addQ(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], 0, task, to, method, -1, -1, false);
						 	} else {
						  		DEBUG(std::cout << "\t\t" << color(YELLOW,"Already known: ") << tasks_per_method[method].first << " " << vertex_to_method[methods_with_two_tasks_vertex[method]] << std::endl);
						  		addQ(tasks_per_method[method].first, methods_with_two_tasks_vertex[method], 0, task, to, method, -1, -1, true);
						  	}
						}
					}
				}
	
				//if (step == 71) exit(0);
				//if (step == 204) exit(0);
	
				step++;
			}
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
	}
	
	std::cout << "Ending ..." << std::endl;
	exit(0);
	//delete sym_vars.manager.release();






	//std::cout << to_string(htn) << std::endl;
}

