#include "sat_planner.h"
#include "sat_encoder.h"
#include "ipasir.h"
#include "pdt.h"
#include "state_formula.h"
#include <cassert>
#include <thread> 
#include <chrono>
#include <pthread.h>
#include <signal.h>

void printSolution(void * solver, Model * htn, PDT* pdt){
	vector<PDT*> leafs;
	pdt->getLeafs(leafs);


	//for (size_t time = 0; time < leafs.size(); time++){
	//	int timeBase = leafs[time]->baseStateVarVariable;

	//	for (int v = 0; v < htn->numVars; v++){
	//		if (htn->firstIndex[v] == htn->lastIndex[v]) continue; // STRIPS
	//		
	//		std::set<int> tru;
	//		for (int f = htn->firstIndex[v]; f <= htn->lastIndex[v]; f++)
	//			if (ipasir_val(solver,timeBase + f) > 0)
	//				tru.insert(f);

	//		if (tru.size() != 1){
	//			cout << "Timestep " << time << endl;
	//			cout << "Variable " << v << " " << htn->varNames[v] << " is not a SAS+ group ..." << endl;
	//			cout << "True are:";
	//			for (int f : tru)
	//				cout << " " << f << " " << htn->factStrs[f];
	//			cout << endl;
	//			
	//			exit(0);
	//		}
	//	}
	//}


	
	int currentID = 0;
	
	cout << "==>" << endl;
	/// extract the primitive plan
	for (PDT* & leaf : leafs){
		for (size_t pIndex = 0; pIndex < leaf->primitiveVariable.size(); pIndex++){
			int prim = leaf->primitiveVariable[pIndex];
			if (ipasir_val(solver,prim) > 0){
				assert(leaf->outputID == -1);
				leaf->outputID = currentID++;
				std::cout << leaf->outputID << " " << htn->taskNames[leaf->possiblePrimitives[pIndex]] << endl;
			}
		}
	}

	// assign numbers to decompositions
	pdt->assignOutputNumbers(solver,currentID, htn);
	cout << "root " << pdt->outputID << endl;

	// out decompositions
	pdt->printDecomposition(htn);
	cout << "<==" << endl;
}

void printVariableTruth(void* solver, Model * htn, sat_capsule & capsule){
	for (int v = 1; v <= capsule.number_of_variables; v++){
		int val = ipasir_val(solver,v);
	
		std::string s = std::to_string(v);
		int x = 4 - s.size();
		while (x-- && x > 0) std::cout << " ";
		std::cout << v << ": ";
		if (val > 0) std::cout << "    ";
		else         std::cout << "not ";
#ifndef NDEBUG
		std::cout << capsule.variableNames[v] << endl; 
#else
		std::cout << v << endl;
#endif
	}
}


void createFormulaForDepth(void* solver, PDT* pdt, Model * htn, sat_capsule & capsule, int depth){
	pdt->expandPDTUpToLevel(depth,htn);

#ifndef NDEBUG
	printPDT(htn,pdt);
#endif
	
	pdt->assignVariableIDs(capsule, htn);
	DEBUG(capsule.printVariables());

	pdt->addDecompositionClauses(solver, capsule);

	// assert the initial abstract task
	assertYes(solver,pdt->abstractVariable[0]);

	// get leafs
	vector<PDT*> leafs;
	pdt->getLeafs(leafs);
	cout << "PDT has " << leafs.size() << " leafs" << endl;
	
	// generate primitive executability formula
	vector<vector<pair<int,int>>> vars;
	get_linear_state_atoms(capsule, leafs, vars);
	generate_state_transition_formula(solver, capsule, vars, leafs, htn);

#ifdef SAT_USEMUTEXES
	generate_mutex_formula(solver,capsule,leafs,htn);
#endif
}


void solve_with_sat_planner_linear_bound_increase(Model * htn){
	PDT* pdt = new PDT(htn);
	sat_capsule capsule;

	int depth = 1;
	while (true){
		void* solver = ipasir_init();
		cout << "Generating formula for depth " << depth << endl;
		createFormulaForDepth(solver,pdt,htn,capsule,depth);
		
		cout << "Formula has " << capsule.number_of_variables << " vars and " << get_number_of_clauses() << " clauses." << endl;
		
		cout << "Starting solver" << endl;
		std::clock_t solver_start = std::clock();
		int state = ipasir_solve(solver);
		std::clock_t solver_end = std::clock();
		double solver_time_in_ms = 1000.0 * (solver_end-solver_start) / CLOCKS_PER_SEC;
		cout << "Solver time: " << solver_time_in_ms << "ms" << endl;
		
		
		cout << "Solver state: " << (state==10?"SAT":"UNSAT") << endl;
		if (state == 10){
#ifndef NDEBUG
			printVariableTruth(solver, htn, capsule);
#endif
			printSolution(solver,htn,pdt);
			ipasir_release(solver);
			return;
		} else
			depth++;
		// release the solver	
		ipasir_release(solver);
	}
}

#define THREAD_PREFIX "\t\t\t\t\t\t\t\t\t"


struct thread_returns{
	Model * htn;
	int depth;
	PDT * pdt;
	void* solver;
	int state;
	
	bool done;

// threading	
	int signal;
	pthread_t tid;
};
const int signalBase = 40;

bool current_done;

void* run_sat_planner_for_depth(void * param){
	thread_returns * ret = (thread_returns*) param;
	cout << THREAD_PREFIX << "Starting Thread for depth " << ret->depth << " @ signal " << ret->signal << endl;
	
	// set this thread to handle the 
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, signalBase + ret->signal);  
	pthread_sigmask(SIG_UNBLOCK, &sigmask, (sigset_t *)0);
	cout << THREAD_PREFIX << "I am handled by " << signalBase + ret->signal << endl;


	ret->pdt = new PDT(ret->htn);
	ret->solver = ipasir_init();

	sat_capsule capsule;
	cout << "Generating formula for depth " << ret->depth << endl;
	createFormulaForDepth(ret->solver,ret->pdt,ret->htn,capsule,ret->depth);
	cout << "Formula has " << capsule.number_of_variables << " vars and " << get_number_of_clauses() << " clauses." << endl;
	
	cout << "Starting solver" << endl;
	std::clock_t solver_start = std::clock();
	ret->state = ipasir_solve(ret->solver);
	std::clock_t solver_end = std::clock();
	double solver_time_in_ms = 1000.0 * (solver_end-solver_start) / CLOCKS_PER_SEC;
	cout << "Solver for depth " << ret->depth << " finished." << endl;
	cout << "Solver time: " << solver_time_in_ms << "ms" << endl;

	cout << "Solver state: " << (ret->state==10?"SAT":"UNSAT") << endl;
	if (ret->state == 10){
#ifndef NDEBUG
		printVariableTruth(ret->solver, ret->htn, capsule);
#endif
		printSolution(ret->solver, ret->htn, ret->pdt);
		ipasir_release(ret->solver);
		exit(0);
	}
	// nothing to return;
	ret->done = true;
	current_done = true; // release the main thread
	return NULL;
}

pthread_mutex_t lock;

int signal_to_release = -1;


void handler(int dummy){
	cout << THREAD_PREFIX << "Suspension signal " << dummy << endl;
	while (true){
		std::this_thread::sleep_for(10ms);
		if (dummy == signal_to_release)
			break;
	}
	cout << THREAD_PREFIX << "Release signal " << dummy << endl;
}

template< class Rep, class Period >
void sleep_until_solver_finished(const std::chrono::duration<Rep, Period>& sleep_duration){
	for (int i = 0; i < 1000 / 25; i++){
		if (current_done) break;
		std::this_thread::sleep_for(25ms);
	}

	current_done = false;
}

void solve_with_sat_planner_time_interleave(Model * htn){
	int maxRuns = 6;
	current_done = false;
	vector<bool> takenSignals (maxRuns);

	sigset_t sigmask;
	struct sigaction action;
	/* Alle Bits auf null setzen */
	sigemptyset(&sigmask);
	/* Signal SIGUSR1 nicht blockieren ... */
	sigaddset(&sigmask, SIGINT);  
	pthread_sigmask(SIG_UNBLOCK, &sigmask, (sigset_t *)0);

   /* Setup Signal-Handler für SIGINT & SIGUSR1 */
	action.sa_flags = 0;
	action.sa_handler = handler;
	for (int i = 0; i < maxRuns; i++){
		cout << "Registering handler for " << signalBase + i << endl;
		sigaction(signalBase + i, &action, (struct sigaction *)0);
	}


	int depth = 1;
	vector<thread_returns*> runs;
	// iterate over time slices
	int positionOnRuns = -1;
	int runningSingal = 0;
	while (true){
		// stop whatever is currently running
		if (positionOnRuns != -1){
			signal_to_release = -1;
			cout << THREAD_PREFIX << "Stopping " << runs[positionOnRuns]->tid << " @ " << runningSingal << endl;
			std::this_thread::sleep_for(1ms);
   			pthread_kill(runs[positionOnRuns]->tid, signalBase + runningSingal);
			std::this_thread::sleep_for(10ms);
		}	

		cout << THREAD_PREFIX << "Switching to next task " << endl;
		do {
			positionOnRuns++;
		} while (runs.size() != positionOnRuns && runs[positionOnRuns]->done);
		cout << THREAD_PREFIX << "Next non-finished task is " << positionOnRuns << endl;

		if (runs.size() == positionOnRuns){ // last run, start a new one
			// check if we are eligible for starting a new run
			int activeRuns = 0;
			for (int i = 0; i < maxRuns; i++) takenSignals[i] = false;
			for (thread_returns * t : runs) if (!t->done){
				activeRuns++;
				takenSignals[t->signal] = true;
			}

			if (activeRuns < maxRuns){
				int firstFreeSignal = -1;
				for (int i = 0; i < maxRuns; i++)
					if (!takenSignals[i]){
						firstFreeSignal = i;
						break;
					}
		
				thread_returns* ret = new thread_returns();
				ret->htn = htn;
				ret->depth = depth++;
				ret->signal = firstFreeSignal;
				ret->done = false;
				runs.push_back(ret);
				
				//void *t1(void *);
				pthread_attr_t attr_obj; 
				pthread_attr_init(&attr_obj);
				pthread_create(&ret->tid, &attr_obj, run_sat_planner_for_depth, (void *)ret);
				cout << THREAD_PREFIX << "Starting worker: " << ret->tid << " @ " << firstFreeSignal << endl;
				
				// get this thread started
				sleep_until_solver_finished(1000ms);
				runningSingal = firstFreeSignal;
				continue;
				//printf("Haupt-Thread(%d) sendet SIGINT an TID(%d)\n", pthread_self(), ret->tid);
   				//pthread_kill(ret->tid, sig+firstFreeSignal);
				//printf("Haupt-Thread(%d) hat gesendet\n", pthread_self());
			} else {
				// it is not possible to start a new thread
				positionOnRuns = -1;
				do {
					positionOnRuns++;
				} while (runs.size() != positionOnRuns && runs[positionOnRuns]->done);
				cout << THREAD_PREFIX << "Not possible to start a new run, next non-finished task is " << positionOnRuns << endl;
			}
		}
		
		runningSingal = runs[positionOnRuns]->signal;
		cout << THREAD_PREFIX << "Letting " << positionOnRuns << " work @ " << runningSingal << endl;
		signal_to_release = signalBase + runningSingal;
		sleep_until_solver_finished(1000ms);
		cout << THREAD_PREFIX << positionOnRuns << " is done working" << endl;
	}
}



void solve_with_sat_planner(Model * htn){
	//htn->writeToPDDL("foo-d.hddl", "foo-p.hddl");

	// start by determining whether this model is totally ordered
	cout << "Instance is totally ordered: " << htn->isTotallyOrdered() << endl;
	// compute transitive closures of all methods
	htn->computeTransitiveClosureOfMethodOrderings();
	htn->buildOrderingDatastructures();


	//solve_with_sat_planner_time_interleave(htn);
	solve_with_sat_planner_linear_bound_increase(htn);
}


void sat_solver_call(){
	cout << ipasir_signature() << endl;
	void* solver = ipasir_init();
	ipasir_add(solver,-1);
	ipasir_add(solver,-2);
	ipasir_add(solver,0);
	
	ipasir_add(solver,-3);
	ipasir_add(solver,2);
	ipasir_add(solver,0);
	
	ipasir_add(solver,3);
	ipasir_add(solver,0);

	int state = ipasir_solve(solver);
	cout << state << endl;
	if (state == 10){
		for (int v = 1; v <= 3; v++)
			cout  << "V " << v << ": " << ipasir_val(solver,v) << endl; 
	}
}
