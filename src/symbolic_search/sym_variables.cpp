#include "sym_variables.h"

#include "../Model.h"

#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string>

using namespace std;

namespace symbolic {

void exceptionError(string /*message*/) {
  // cout << message << endl;
  throw BDDError();
}

SymVariables::SymVariables(progression::Model *model)
    : model(model), cudd_init_nodes(16000000L), cudd_init_cache_size(16000000L),
      cudd_init_available_memory(0L), var_ordering(false) {}

void SymVariables::init(bool aux_variables) {
  vector<int> var_order;
  if (var_ordering) {
    // InfluenceGraph::compute_gamer_ordering(var_order);
  } else {
    for (int i = 0; i < model->numVars; ++i) {
      var_order.push_back(i);
    }
  }
  cout << "Sym variable order: ";
  for (int v : var_order)
    cout << v << " ";
  cout << endl;

  init(var_order, aux_variables);
}

// Constructor that makes use of global variables to initialize the
// symbolic_search structures

void SymVariables::init(const vector<int> &v_order, bool aux_variables) {
  if (!aux_variables) {
    cout << "Initializing Symbolic Variables" << endl;
  } else {
    cout << "Initializing Symbolic Variables (incl. auxillary Variables)"
         << endl;
  }
  var_order = vector<int>(v_order);
  int num_fd_vars = var_order.size();

  // Initialize binary representation of variables.
  numBDDVars = 0;
  bdd_index_pre = vector<vector<int>>(v_order.size());
  bdd_index_eff = vector<vector<int>>(v_order.size());

  if (aux_variables) {
    bdd_index_aux = vector<vector<int>>(v_order.size());
  }
  int _numBDDVars = 0; // numBDDVars;
  for (int var : var_order) {
    int var_len = ceil(log2(getDomainSize(var)));
    numBDDVars += var_len;
    for (int j = 0; j < var_len; j++) {
      bdd_index_pre[var].push_back(_numBDDVars);
      bdd_index_eff[var].push_back(_numBDDVars + 1);

      if (aux_variables) {
        bdd_index_aux[var].push_back(_numBDDVars + 2);
        _numBDDVars += 3;

      } else {
        _numBDDVars += 2;
      }
    }
  }
  cout << "Num variables: " << var_order.size() << " => " << numBDDVars << " ["
       << _numBDDVars << "]" << endl;

  // Initialize manager
  cout << "Initialize Symbolic Manager(" << _numBDDVars << ", "
       << cudd_init_nodes / _numBDDVars << ", " << cudd_init_cache_size << ", "
       << cudd_init_available_memory << ")" << endl;
  manager = unique_ptr<Cudd>(
      new Cudd(_numBDDVars, 0, cudd_init_nodes / _numBDDVars,
               cudd_init_cache_size, cudd_init_available_memory));

  manager->setHandler(exceptionError);
  manager->setTimeoutHandler(exceptionError);
  manager->setNodesExceededHandler(exceptionError);

  cout << "Generating binary variables" << endl;
  // Generate binary_variables
  for (int i = 0; i < _numBDDVars; i++) {
    variables.push_back(manager->bddVar(i));
  }

  preconditionBDDs.resize(num_fd_vars);
  effectBDDs.resize(num_fd_vars);
  if (aux_variables) {
    auxBDDs.resize(num_fd_vars);
  }
  biimpBDDs.resize(num_fd_vars);
  validValues.resize(num_fd_vars);
  validBDD = oneBDD();
  // Generate predicate (precondition (s) and effect (s')) BDDs
  for (int var : var_order) {
    for (int j = 0; j < getDomainSize(var); j++) {
      preconditionBDDs[var].push_back(createPreconditionBDD(var, j));
      effectBDDs[var].push_back(createEffectBDD(var, j));
      if (aux_variables) {
        auxBDDs[var].push_back(createAuxBDD(var, j));
      }
    }
    validValues[var] = zeroBDD();
    for (int j = 0; j < getDomainSize(var); j++) {
      validValues[var] += preconditionBDDs[var][j];
    }
    validBDD *= validValues[var];
    biimpBDDs[var] =
        createBiimplicationBDD(bdd_index_pre[var], bdd_index_eff[var]);

    if (aux_variables) {
      auxBiimpBDDs.resize(num_fd_vars);
      auxBiimpBDDs[var] =
          createBiimplicationBDD(bdd_index_pre[var], bdd_index_aux[var]);
    }
  }

  for (int var : var_order) {
    for (int bdd_var : vars_index_pre(var)) {
      swapVarsPre.push_back(bddVar(bdd_var));
    }
    for (int bdd_var : vars_index_eff(var)) {
      swapVarsEff.push_back(bddVar(bdd_var));
    }
    for (int bdd_var : vars_index_aux(var)) {
      swapVarsAux.push_back(bddVar(bdd_var));
    }
  }

  existsVarsPre = oneBDD();
  existsVarsEff = oneBDD();
  existsVarsAux = oneBDD();

  for (size_t i = 0; i < swapVarsPre.size(); ++i) {
    existsVarsPre *= swapVarsPre[i];
    existsVarsEff *= swapVarsEff[i];
    existsVarsAux *= swapVarsAux[i];
  }

  cout << "Symbolic Variables... Done." << endl;
}

BDD SymVariables::getStateBDD(const std::vector<int> &state) const {
  BDD res = oneBDD();
  for (int i = var_order.size() - 1; i >= 0; i--) {
    res = res * preconditionBDDs[var_order[i]][state[var_order[i]]];
  }
  return res;
}

BDD SymVariables::getStateBDD(const int *state_bits,
                              int state_bits_size) const {
  BDD res = oneBDD();
  unordered_set<int> contained_vars;
  for (int i = 0; i < state_bits_size; i++) {
    int var = model->varOfStateBit[state_bits[i]];
    int val = state_bits[i] - model->firstIndex[var];
    res = res * preconditionBDDs[var_order[var]][val];
    contained_vars.insert(var);
  }

  for (int var = 0; var < model->numVars; ++var) {
    if (contained_vars.count(var) == 0) {
      res *= preconditionBDDs[var_order[var]][1];
    }
  }

  return res;
}



BDD SymVariables::getStateBDD(std::vector<bool> state_vec) const {
  BDD res = oneBDD();
  unordered_set<int> contained_vars;
  for (int i = 0; i < state_vec.size(); i++) {
	if (state_vec[i] == false) continue;
    int var = model->varOfStateBit[i];
    int val = i - model->firstIndex[var];
    res = res * preconditionBDDs[var_order[var]][val];
    contained_vars.insert(var);
  }

  for (int var = 0; var < model->numVars; ++var) {
    if (contained_vars.count(var) == 0) {
      res *= preconditionBDDs[var_order[var]][1];
    }
  }


  return res;
}


BDD SymVariables::getPartialStateBDD(const int *state_bits,
                              int state_bits_size) const {
  BDD res = oneBDD();
  for (int i = 0; i < state_bits_size; i++) {
    int var = model->varOfStateBit[state_bits[i]];
    int val = state_bits[i] - model->firstIndex[var];
    res = res * preconditionBDDs[var_order[var]][val];
  }

  return res;
}

/*BDD SymVariables::getStateBDD(const GlobalState &state) const {
  BDD res = oneBDD();
  for (int i = var_order.size() - 1; i >= 0; i--) {
    res = res * preconditionBDDs[var_order[i]][state[var_order[i]]];
  }
  return res;
}*/

BDD SymVariables::getPartialStateBDD(
    const vector<pair<int, int>> &state) const {
  BDD res = validBDD;
  for (int i = state.size() - 1; i >= 0; i--) {
    // if(find(var_order.begin(), var_order.end(),
    //               state[i].first) != var_order.end()) {
    res = res * preconditionBDDs[state[i].first][state[i].second];
    //}
  }
  return res;
}

BDD SymVariables::generateBDDVar(const std::vector<int> &_bddVars,
                                 int value) const {
  BDD res = oneBDD();
  for (int v : _bddVars) {
    if (value % 2) { // Check if the binary variable is asserted or negated
      res = res * variables[v];
    } else {
      res = res * (!variables[v]);
    }
    value /= 2;
  }
  return res;
}

BDD SymVariables::createBiimplicationBDD(const std::vector<int> &vars,
                                         const std::vector<int> &vars2) const {
  BDD res = oneBDD();
  for (size_t i = 0; i < vars.size(); i++) {
    res *= variables[vars[i]].Xnor(variables[vars2[i]]);
  }
  return res;
}

vector<BDD> SymVariables::getBDDVars(const vector<int> &vars,
                                     const vector<vector<int>> &v_index) const {
  vector<BDD> res;
  for (int v : vars) {
    for (int bddv : v_index[v]) {
      res.push_back(variables[bddv]);
    }
  }
  return res;
}

int SymVariables::getDomainSize(int var) const {
  int var_domain_size = 0;

  // Check for domain size (boolean have same index)
  if (model->firstIndex[var] == model->lastIndex[var]) {
    var_domain_size = 2;
  } else {
    var_domain_size = model->lastIndex[var] - model->firstIndex[var] + 1;
  }
  return var_domain_size;
}

bool SymVariables::isStripsVariable(int var) const {
  return model->firstIndex[var] == model->lastIndex[var];
}

BDD SymVariables::getCube(int var, const vector<vector<int>> &v_index) const {
  BDD res = oneBDD();
  for (int bddv : v_index[var]) {
    res *= variables[bddv];
  }
  return res;
}

BDD SymVariables::getCube(const set<int> &vars,
                          const vector<vector<int>> &v_index) const {
  BDD res = oneBDD();
  for (int v : vars) {
    for (int bddv : v_index[v]) {
      res *= variables[bddv];
    }
  }
  return res;
}

std::vector<std::string> SymVariables::get_fd_variable_names() const {
  int num_vars = auxBDDs.empty() ? numBDDVars * 2 : numBDDVars * 3;
  std::vector<string> var_names(num_vars);
  for (int v : var_order) {
    int exp = 0;
    for (int j : bdd_index_pre[v]) {
      var_names[j] = "var" + to_string(v) + "_2^" + std::to_string(exp);
      var_names[j + 1] =
          "var" + to_string(v) + "_2^" + std::to_string(exp) + "_primed";
      if (!auxBDDs.empty()) {
        var_names[j + 2] =
            "var" + to_string(v) + "_2^" + std::to_string(exp) + "_aux";
      }
      exp++;
    }
  }

  return var_names;
}

void SymVariables::print_options() const {
  cout << "CUDD Init: nodes=" << cudd_init_nodes
       << " cache=" << cudd_init_cache_size
       << " max_memory=" << cudd_init_available_memory
       << " ordering: " << (var_ordering ? "special" : "standard") << endl;
}

void SymVariables::bdd_to_dot(const BDD &bdd,
                              const std::string &file_name) const {
  int num_vars = auxBDDs.empty() ? numBDDVars * 2 : numBDDVars * 3;
  std::vector<string> var_names(num_vars);
  for (int v : var_order) {
    int exp = 0;
    for (int j : bdd_index_pre[v]) {
      var_names[j] = "var" + to_string(v) + "_2^" + std::to_string(exp);
      var_names[j + 1] =
          "var" + to_string(v) + "_2^" + std::to_string(exp) + "_primed";
      if (!auxBDDs.empty()) {
        var_names[j + 2] =
            "var" + to_string(v) + "_2^" + std::to_string(exp) + "_aux";
      }
      exp++;
    }
  }

  std::vector<char *> names(num_vars);
  for (int i = 0; i < num_vars; ++i) {
    names[i] = &var_names[i].front();
  }
  FILE *outfile = fopen(file_name.c_str(), "w");
  DdNode **ddnodearray = (DdNode **)malloc(sizeof(bdd.Add().getNode()));
  ddnodearray[0] = bdd.Add().getNode();
  Cudd_DumpDot(manager->getManager(), 1, ddnodearray, names.data(), NULL,
               outfile); // dump the function to .dot file
  free(ddnodearray);
  fclose(outfile);
}

/**
 * Return the first state that is not in conflict with the given BDD.
 */
std::vector<int> SymVariables::getStateFrom(const BDD &bdd) const {
  std::vector<int> vals;
  BDD current = bdd;
  for (int var = 0; var < preconditionBDDs.size(); var++) {
    for (int val = 0; val < preconditionBDDs[var].size(); val++) {
      BDD aux = current * preconditionBDDs[var][val];
      if (!aux.IsZero()) {
        current = aux;
        vals.push_back(val);
        break;
      }
    }
  }
  return vals;
}

/**
 * Return all states that are contained in the given BDD.
 * (Only for debugging, can be very expensive!
 */
std::vector<vector<int>> SymVariables::getStatesFrom(const BDD &bdd) const {
  std::vector<vector<int>> res;
  BDD cur = bdd;
  for (size_t i=0; i < numStates(bdd); i++) {
    vector<int> state = getStateFrom(cur);
        cur *= !getStateBDD(state);

        vector<int> retState;
        for (int var = 0; var < preconditionBDDs.size(); var++){
                if (state[var] > model->lastIndex[var] - model->firstIndex[var]) continue;
                retState.push_back(state[var] + model->firstIndex[var]);
        }

        res.push_back(retState);

    if (cur.IsZero())
      return res;
  }
  return res;
}



} // namespace symbolic
