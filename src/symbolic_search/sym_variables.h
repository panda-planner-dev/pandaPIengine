#ifndef SYM_VARIABLES_H
#define SYM_VARIABLES_H

#include "cuddObj.hh"

#include <memory>
#include <set>
#include <vector>

namespace progression {
class Model;
}

namespace symbolic {

/*
 * BDD-Variables for a symbolic exploration.
 * This information is global for every class using symbolic search.
 * The only decision fixed here is the variable ordering, which is assumed to be
 * always fixed.
 */
struct BDDError {};
extern void exceptionError(std::string message);

class SymVariables {
  progression::Model *model;
  // Var order used by the algorithm.
  // const VariableOrderType variable_ordering;
  // Parameters to initialize the CUDD manager
  const long cudd_init_nodes;            // Number of initial nodes
  const long cudd_init_cache_size;       // Initial cache size
  const long cudd_init_available_memory; // Maximum available memory (bytes)
  const bool var_ordering;

public:
  std::unique_ptr<Cudd> manager; // manager associated with this symbolic search

  int numBDDVars; // Number of binary variables (just one set, the total number
  // is numBDDVars*2
  std::vector<BDD> variables; // BDD variables

  // The variable order must be complete.
  std::vector<int> var_order; // Variable(FD) order in the BDD
  std::vector<std::vector<int>> bdd_index_pre, bdd_index_eff,
      bdd_index_aux; // vars(BDD) for each var(FD)

  std::vector<std::vector<BDD>>
      preconditionBDDs; // BDDs associated with the precondition of a predicate
  std::vector<std::vector<BDD>>
      effectBDDs; // BDDs associated with the effect of a predicate
  std::vector<BDD>
      biimpBDDs; // BDDs associated with the biimplication of one variable(FD)
  std::vector<BDD>
      validValues; // BDD that represents the valid values of all the variables
  BDD validBDD;    // BDD that represents the valid values of all the variables

  std::vector<std::vector<BDD>>
      auxBDDs; // BDDs associated with auxillary conditions a predicate (e.g.
               // for fixpoint computations)
  std::vector<BDD>
      auxBiimpBDDs; // BDDs associated with the biimplication of
                    // one variable(FD) from effect to aux variables

  std::vector<BDD> swapVarsPre, swapVarsEff, swapVarsAux;
  BDD existsVarsPre;
  BDD existsVarsEff;
  BDD existsVarsAux;

  void init(const std::vector<int> &v_order, bool aux_variables);

public:
  SymVariables(progression::Model *model);
  void init(bool aux_variables);

  // State getStateFrom(const BDD & bdd) const;
  BDD getStateBDD(const std::vector<int> &state) const;
  BDD getStateBDD(const int *state_bits, int state_bits_size) const;
  BDD getStateBDD(std::vector<bool> state_vec) const;

  BDD getPartialStateBDD(const std::vector<std::pair<int, int>> &state) const;
  BDD getPartialStateBDD(const int *state_bits, int state_bits_size) const;

  inline const std::vector<int> &vars_index_pre(int variable) const {
    return bdd_index_pre[variable];
  }

  inline const std::vector<int> &vars_index_eff(int variable) const {
    return bdd_index_eff[variable];
  }

  inline const std::vector<int> &vars_index_aux(int variable) const {
    return bdd_index_aux[variable];
  }

  inline const BDD &preBDD(int variable, int value) const {
    return preconditionBDDs[variable][value];
  }

  inline const BDD &effBDD(int variable, int value) const {
    return effectBDDs[variable][value];
  }

  inline const BDD &auxBDD(int variable, int value) const {
    return auxBDDs[variable][value];
  }

  inline BDD getCubePre(int var) const { return getCube(var, bdd_index_pre); }

  inline BDD getCubePre(const std::set<int> &vars) const {
    return getCube(vars, bdd_index_pre);
  }

  inline BDD getCubeEff(int var) const { return getCube(var, bdd_index_eff); }

  inline BDD getCubeEff(const std::set<int> &vars) const {
    return getCube(vars, bdd_index_eff);
  }

  inline BDD getCubeAux(int var) const { return getCube(var, bdd_index_aux); }

  inline BDD getCubeAux(const std::set<int> &vars) const {
    return getCube(vars, bdd_index_aux);
  }

  inline const BDD &biimp(int variable) const { return biimpBDDs[variable]; }

  inline const BDD &auxBiimp(int variable) const {
    return auxBiimpBDDs[variable];
  }

  inline std::vector<BDD> getBDDVarsPre() const {
    return getBDDVars(var_order, bdd_index_pre);
  }

  inline std::vector<BDD> getBDDVarsEff() const {
    return getBDDVars(var_order, bdd_index_eff);
  }

  inline std::vector<BDD> getBDDVarsAux() const {
    return getBDDVars(var_order, bdd_index_aux);
  }

  inline std::vector<BDD> getBDDVarsPre(const std::vector<int> &vars) const {
    return getBDDVars(vars, bdd_index_pre);
  }

  inline std::vector<BDD> getBDDVarsEff(const std::vector<int> &vars) const {
    return getBDDVars(vars, bdd_index_eff);
  }

  inline std::vector<BDD> getBDDVarsAux(const std::vector<int> &vars) const {
    return getBDDVars(vars, bdd_index_aux);
  }

  inline BDD zeroBDD() const {
    return manager->bddZero();
    ;
  }

  inline BDD oneBDD() const {
    return manager->bddOne();
    ;
  }

  inline BDD validStates() const { return validBDD; }

  inline BDD bddVar(int index) const { return variables[index]; }

  inline void setTimeLimit(int maxTime) {
    manager->SetTimeLimit(maxTime);
    manager->ResetStartTime();
  }

  inline void unsetTimeLimit() { manager->UnsetTimeLimit(); }

  std::vector<std::string> get_fd_variable_names() const;

  void print_options() const;

  void bdd_to_dot(const BDD &bdd, const std::string &file_name) const;

  int getDomainSize(int var) const;

  bool isStripsVariable(int var) const;

  double numStates(const BDD &bdd) const {
    return bdd.CountMinterm(numBDDVars);
  }

  std::vector<std::vector<int>> getStatesFrom(const BDD &bdd) const;

private:
  // Auxiliar function helping to create precondition and effect BDDs
  // Generates value for bddVars.
  BDD generateBDDVar(const std::vector<int> &_bddVars, int value) const;
  BDD getCube(int var, const std::vector<std::vector<int>> &v_index) const;
  BDD getCube(const std::set<int> &vars,
              const std::vector<std::vector<int>> &v_index) const;
  BDD createBiimplicationBDD(const std::vector<int> &vars,
                             const std::vector<int> &vars2) const;
  std::vector<BDD>
  getBDDVars(const std::vector<int> &vars,
             const std::vector<std::vector<int>> &v_index) const;

  inline BDD createPreconditionBDD(int variable, int value) const {
    return generateBDDVar(bdd_index_pre[variable], value);
  }

  inline BDD createEffectBDD(int variable, int value) const {
    return generateBDDVar(bdd_index_eff[variable], value);
  }

  inline BDD createAuxBDD(int variable, int value) const {
    return generateBDDVar(bdd_index_aux[variable], value);
  }

  inline int getNumBDDVars() const { return numBDDVars; }

  std::vector<int> getStateFrom(const BDD &bdd) const;
};
} // namespace symbolic

#endif
