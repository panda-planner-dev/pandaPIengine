#ifndef TRANSITION_RELATION_H
#define TRANSITION_RELATION_H

#include "sym_variables.h"

#include <set>
#include <vector>

namespace symbolic {

/*
 * Represents a symbolic transition.
 * It has two differentiated parts: label and abstract state transitions
 * Label refers to variables not considered in the merge-and-shrink
 * Each label has one or more abstract state transitions
 */
class TransitionRelation {

  SymVariables *sV; // To call basic BDD creation methods
  int cost;         // transition cost
  BDD tBDD;         // bdd for making the relprod

  std::vector<int> effVars;     // FD Index of eff variables. Must be sorted!!
  BDD existsVars, existsBwVars; // Cube with variables to existentialize
  std::vector<BDD> swapVarsS,
      swapVarsSp; // Swap variables s to sp and viceversa

  std::set<int> ops_ids; // List of operators represented by the TR

public:
  // Constructor for transitions irrelevant for the abstraction
  TransitionRelation(SymVariables *sVars, int op_id, int cost);
  void init(progression::Model *model);

  // Copy constructor
  TransitionRelation(const TransitionRelation &) = default;

  BDD image(const BDD &from) const;
  BDD preimage(const BDD &from) const;
  BDD image(const BDD &from, int maxNodes) const;
  BDD preimage(const BDD &from, int maxNodes) const;

  /*void edeletion(const std::vector<std::vector<BDD>> &notMutexBDDsByFluentFw,
                 const std::vector<std::vector<BDD>> &notMutexBDDsByFluentBw,
                 const std::vector<std::vector<BDD>> &exactlyOneBDDsByFluent);*/

  void merge(const TransitionRelation &t2, int maxNodes);

  int getCost() const { return cost; }

  void set_cost(int cost_) { cost = cost_; }

  int nodeCount() const { return tBDD.nodeCount(); }

  const std::set<int> &getOpsIds() const { return ops_ids; }

  const BDD &getBDD() const { return tBDD; }
};
} // namespace symbolic
#endif