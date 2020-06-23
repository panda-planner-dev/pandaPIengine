#include "transition_relation.h"

#include "../Model.h"

#include <algorithm>
#include <cassert>
#include <map>

using namespace std;

namespace symbolic {

TransitionRelation::TransitionRelation(SymVariables *sVars, int op_id, int cost)
    : sV(sVars), cost(cost), tBDD(sVars->oneBDD()), existsVars(sVars->oneBDD()),
      existsBwVars(sVars->oneBDD()) {
  ops_ids.insert(op_id);
}

void TransitionRelation::init(progression::Model *model) {
  int op_id = *ops_ids.begin();

  for (int pre_id = 0; pre_id < model->numPrecs[op_id]; ++pre_id) {
    int var = model->varOfStateBit[model->precLists[op_id][pre_id]];
    int val = model->precLists[op_id][pre_id] - model->firstIndex[var];
    tBDD *= sV->preBDD(var, val);
  }

  map<int, BDD> effect_conditions;
  map<int, BDD> effects;

  // Encode add effect with conditon effect
  for (int add_id = 0; add_id < model->numAdds[op_id]; ++add_id) {
    int var = model->varOfStateBit[model->addLists[op_id][add_id]];
    int val = model->addLists[op_id][add_id] - model->firstIndex[var];
    if (std::find(effVars.begin(), effVars.end(), var) == effVars.end()) {
      effVars.push_back(var);
    }

    effect_conditions[var] = sV->zeroBDD();
    effects[var] = sV->effBDD(var, val);
  }

  // Encode del effect with conditon effect
  for (int del_id = 0; del_id < model->numDels[op_id]; ++del_id) {
    int var = model->varOfStateBit[model->delLists[op_id][del_id]];
    int val = model->delLists[op_id][del_id] - model->firstIndex[var];
    if (!sV->isStripsVariable(var))
      continue;
    if (std::find(effVars.begin(), effVars.end(), var) == effVars.end()) {
      effVars.push_back(var);
    }

    effect_conditions[var] = sV->zeroBDD();
    effects[var] = !sV->effBDD(var, val);
  }

  // Add effects to the tBDD
  int counter = 0;
  for (auto it = effects.rbegin(); it != effects.rend(); ++it) {
    int var = it->first;
    BDD effectBDD = it->second;
    // If some possibility is not covered by the conditions of the
    // conditional effect, then in those cases the value of the value
    // is preserved with a biimplication
    if (!effect_conditions[var].IsZero()) {
      effectBDD += (effect_conditions[var] * sV->biimp(var));
    }
    tBDD *= effectBDD;
    counter++;
  }
  if (tBDD.IsZero()) {
    cerr << "Operator is empty: ID " << op_id << endl;
    // exit(0);
  }

  sort(effVars.begin(), effVars.end());
  for (int var : effVars) {
    for (int bdd_var : sV->vars_index_pre(var)) {
      swapVarsS.push_back(sV->bddVar(bdd_var));
    }
    for (int bdd_var : sV->vars_index_eff(var)) {
      swapVarsSp.push_back(sV->bddVar(bdd_var));
    }
  }
  assert(swapVarsS.size() == swapVarsSp.size());
  // existsVars/existsBwVars is just the conjunction of swapVarsS and swapVarsSp
  for (size_t i = 0; i < swapVarsS.size(); ++i) {
    existsVars *= swapVarsS[i];
    existsBwVars *= swapVarsSp[i];
  }
}

BDD TransitionRelation::image(const BDD &from) const {
  BDD aux = from;
  BDD tmp = tBDD.AndAbstract(aux, existsVars);
  BDD res = tmp.SwapVariables(swapVarsS, swapVarsSp);
  return res;
}

BDD TransitionRelation::image(const BDD &from, int maxNodes) const {
  BDD aux = from;
  BDD tmp = tBDD.AndAbstract(aux, existsVars, maxNodes);
  BDD res = tmp.SwapVariables(swapVarsS, swapVarsSp);
  return res;
}

BDD TransitionRelation::preimage(const BDD &from) const {
  BDD tmp = from.SwapVariables(swapVarsS, swapVarsSp);
  BDD res = tBDD.AndAbstract(tmp, existsBwVars);
  return res;
}

BDD TransitionRelation::preimage(const BDD &from, int maxNodes) const {
  BDD tmp = from.SwapVariables(swapVarsS, swapVarsSp);
  BDD res = tBDD.AndAbstract(tmp, existsBwVars, maxNodes);
  return res;
}

void TransitionRelation::merge(const TransitionRelation &t2, int maxNodes) {
  assert(cost == t2.cost);
  if (cost != t2.cost) {
    cout << "Error: merging transitions with different cost: " << cost << " "
         << t2.cost << endl;
  }

  //  cout << "set_union" << endl;
  // Attempt to generate the new tBDD
  vector<int> newEffVars;
  set_union(effVars.begin(), effVars.end(), t2.effVars.begin(),
            t2.effVars.end(), back_inserter(newEffVars));

  BDD newTBDD = tBDD;
  BDD newTBDD2 = t2.tBDD;

  //    cout << "Eff vars" << endl;
  vector<int>::const_iterator var1 = effVars.begin();
  vector<int>::const_iterator var2 = t2.effVars.begin();
  for (vector<int>::const_iterator var = newEffVars.begin();
       var != newEffVars.end(); ++var) {
    if (var1 == effVars.end() || *var1 != *var) {
      newTBDD *= sV->biimp(*var);
    } else {
      ++var1;
    }

    if (var2 == t2.effVars.end() || *var2 != *var) {
      newTBDD2 *= sV->biimp(*var);
    } else {
      ++var2;
    }
  }
  newTBDD = newTBDD.Or(newTBDD2, maxNodes);

  if (newTBDD.nodeCount() > maxNodes) {
    throw BDDError(); // We could not sucessfully merge
  }

  tBDD = newTBDD;

  effVars.swap(newEffVars);
  existsVars *= t2.existsVars;
  existsBwVars *= t2.existsBwVars;

  for (size_t i = 0; i < t2.swapVarsS.size(); i++) {
    if (find(swapVarsS.begin(), swapVarsS.end(), t2.swapVarsS[i]) ==
        swapVarsS.end()) {
      swapVarsS.push_back(t2.swapVarsS[i]);
      swapVarsSp.push_back(t2.swapVarsSp[i]);
    }
  }

  ops_ids.insert(t2.ops_ids.begin(), t2.ops_ids.end());
}

// For each op, include relevant mutexes

/*void TransitionRelation::edeletion(
    const std::vector<std::vector<BDD>> &notMutexBDDsByFluentFw,
    const std::vector<std::vector<BDD>> &notMutexBDDsByFluentBw,
    const std::vector<std::vector<BDD>> &exactlyOneBDDsByFluent) {
  assert(ops_ids.size() == 1);

  TaskProxy task_proxy(*tasks::g_root_task);
  // For each op, include relevant mutexes
  for (const OperatorID &op_id : ops_ids) {
    OperatorProxy op = task_proxy.get_operators()[op_id.get_index()];
    for (const auto &eff : op.get_effects()) {
      FactPair pp = eff.get_fact().get_pair();
      // TODO: somehow fix this here
      FactPair pre(-1, -1);
      for (const auto &cond : op.get_preconditions()) {
        if (cond.get_pair().var == pp.var) {
          pre = cond.get_pair();
          break;
        }
      }

      // edeletion bw
      if (pre.var == -1) {
        // We have a post effect over this variable.
        // That means that every previous value is possible
        // for each value of the variable
        for (int val = 0;
             val < tasks::g_root_task->get_variable_domain_size(pp.var);
             val++) {
          tBDD *= notMutexBDDsByFluentBw[pp.var][val];
        }
      } else {
        // In regression, we are making true pp.pre
        // So we must negate everything of these.
        tBDD *= notMutexBDDsByFluentBw[pp.var][pre.value];
      }
      // TODO(speckd): Here we need to swap in the correct direction!
      // edeletion fw
      tBDD *= notMutexBDDsByFluentFw[pp.var][pp.value].SwapVariables(
          swapVarsS, swapVarsSp);

      // edeletion invariants
      tBDD *= exactlyOneBDDsByFluent[pp.var][pp.value];
    }
  }
}*/

} // namespace symbolic
