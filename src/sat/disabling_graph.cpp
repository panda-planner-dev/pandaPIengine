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

bool are_actions_applicable_in_the_same_state(Model * htn, int a, int b){
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

void compute_disabling_graph(Model * htn){
	cout << endl << "Computing Disabling Graph" << endl;
	std::clock_t dg_start = std::clock();

	vector<unordered_set<int>> tempAdj (htn->numActions);
	int edge = 0;
	for (int f = 0; f < htn->numStateBits; f++){
		for (int deletingIndex = 0; deletingIndex < htn->delToActionSize[f]; deletingIndex++){
			int deletingAction = htn->delToAction[f][deletingIndex];
			
			for (int needingIndex = 0; needingIndex < htn->precToActionSize[f]; needingIndex++){
				int needingAction = htn->precToAction[f][needingIndex];
				if (deletingAction == needingAction) continue; // action cannot disable itself
				if (!are_actions_applicable_in_the_same_state(htn, deletingAction, needingAction)) continue;
				/*DEBUG(
					cout << deletingAction << " " << htn->taskNames[deletingAction];
					cout << " vs " << needingAction << " " << htn->taskNames[needingAction] << endl;
					);*/

				tempAdj[deletingAction].insert(needingAction);
			}
		}
	}
	
	// convert into int data structures
	graph * dg = new graph(tempAdj);

	std::clock_t dg_end = std::clock();
	double dg_time = 1000.0 * (dg_end-dg_start) / CLOCKS_PER_SEC;
	cout << "Generated graph with " << dg->count_edges() << " edges." << endl;
	cout << "Generating the graph took " << dg_time << "ms." << endl;
}

#ifdef FALSE

  // the non-extended disabling graph will contain only those edges implies by the problem itself, not by additional constraints (like LTL)
  lazy val (disablingGraph, nonExtendedDisablingGraph): (DirectedGraph[Task], DirectedGraph[Task]) = {
    println("Computing disabling graph")
    val time1 = System.currentTimeMillis()

    def applicable(task1: IntTask, task2: IntTask): Boolean = {
      var counter = false
      // incompatibe preconditions via invariants
      var i = 0
      while (!counter && i < task1.preList.length) {
        var j = 0
        while (!counter && j < task2.preList.length) {
          counter |= checkInvariant(-task1.preList(i), -task2.preList(j))
          j += 1
        }
        i += 1
      }
      // incompatible effects via invariants
      i = 0
      while (!counter && i < task1.invertedEffects.length) {
        var j = 0
        while (!counter && j < task2.invertedEffects.length) {
          counter |= checkInvariant(task1.invertedEffects(i), task2.invertedEffects(j))
          j += 1
        }
        i += 1
      }
      // are applicable if we have not found a counter example
      !counter
    }

    def affects(task1: Task, task2: Task): Boolean = task1.delEffectsAsPredicate exists task2.posPreconditionAsPredicateSet.contains

    // compute affection
    val predicateToAdding: Map[Predicate, Array[IntTask]] =
      intTasks flatMap { t => t.task.addEffectsAsPredicate map { e => (t, e) } } groupBy (_._2) map { case (p, as) => p -> as.map(_._1) }

    val predicateToDeleting: Map[Predicate, Array[IntTask]] =
      intTasks flatMap { t => t.task.delEffectsAsPredicate map { e => (t, e) } } groupBy (_._2) map { case (p, as) => p -> as.map(_._1) }

    val predicateToNeeding: Map[Predicate, Array[IntTask]] =
      intTasks flatMap { t => t.task.posPreconditionAsPredicate map { e => (t, e) } } groupBy (_._2) map { case (p, as) => p -> as.map(_._1) }

    val edgesWithDuplicats: Seq[(IntTask, IntTask)] =
      predicateToDeleting.toSeq flatMap { case (p, as) => predicateToNeeding.getOrElse(p, new Array(0)) flatMap { n => as map { d => (d, n) } } }
    val alwaysEdges: Seq[(IntTask, IntTask)] = (edgesWithDuplicats groupBy { _._1 } toSeq) flatMap { case (t1, t2) => (t2 map {
      _._2
    } distinct) collect { case t if t != t1 => (t1, t) }
    }
    val additionalEdges = additionalEdgesInDisablingGraph.flatMap(_.additionalEdges(this)(predicateToAdding, predicateToDeleting, predicateToNeeding))
    val time12 = System.currentTimeMillis()
    println("Candidates (" + alwaysEdges.length + " & " + additionalEdges.length + ") generated: " + ((time12 - time1) / 1000))

    val alwaysApplicableEdges = alwaysEdges collect { case (a, b) if applicable(a, b) => (a.task, b.task) }
    val additionalApplicableEdges = additionalEdges collect { case (a, b) if applicable(a, b) => (a.task, b.task) }

    val fullDG = SimpleDirectedGraph(domain.primitiveTasks, (alwaysApplicableEdges ++ additionalApplicableEdges).distinct)
    val nonExtendedDG = SimpleDirectedGraph(domain.primitiveTasks, alwaysApplicableEdges.distinct)

    val time2 = System.currentTimeMillis()
    println("EDGELIST " + fullDG.edgeList.length + " of " + fullDG.vertices.size * (fullDG.vertices.size - 1) + " in " + (time2 - time1) / 1000.0)
    val allSCCS = fullDG.stronglyConnectedComponents
    val time3 = System.currentTimeMillis()
    println(((allSCCS map { _.size } groupBy { x => x }).toSeq.sortBy(_._1) map { case (k, s) => s.size + "x" + k } mkString ", ") + " in " + (time3 - time2) / 1000.0)

    (fullDG, nonExtendedDG)
  }


#endif


