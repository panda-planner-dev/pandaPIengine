/*
 * flags.h
 *
 *  Created on: 26.09.2017
 *      Author: Daniel Höller
 */

#ifndef FLAGS_H_
#define FLAGS_H_


// constants
#define UNREACHABLE INT_MAX
#define NOACTION -1
#define FORBIDDEN -2

// [heuristics]
#define ZERO 0      // hVal is zero for every node
#define RCFILTER 1  // relaxed composition heuristic with the filter heuristic
#define RCFF 2      // relaxed composition heuristic with the FF heuristic
#define RCADD 3     // relaxed composition heuristic with the add heuristic
#define RCLMC 4     // relaxed composition heuristic with the LM-Cut heuristic
#define DOFREEILP 5 // delete and ordering free heuristic (exact ilp calculation)
#define DOFREELP 6  // delete and ordering free heuristic (approximate lp calculation)
#define LMCLOCAL 7
#define LMCANDOR 8
#define LMCFD 9

#define RCFILTER2 10
#define RCFF2 11
#define RCADD2 12
#define RCLMC2 13

// [state-representation]
#define SRCOPY 0  // copy bit vector that represents state
#define SRCALC1 1 // calculate state based on current plan
#define SRCALC2 2 // calculate state based on current plan
#define SRLIST 3  // maintain int list with bits currently set

// [search-type]
#define DFSEARCH 0
#define BFSEARCH 1
#define HEURISTICSEARCH 2

// [algorithm]
#define PROGRESSIONORG 0 // branches over abstract and primitive tasks
#define ICAPS18 1        // branches over primitive and one abstract task
#define JAIR19 2         // processes abstract tasks first

// *****************
// * Configuration *
// *****************

// time limit in seconds
#define TIMELIMIT 600

//#define SEARCHALG PROGRESSIONORG
//#define SEARCHALG ICAPS18
#define SEARCHALG JAIR19 // choose from [algorithm]

#define EARLYGOALTEST

// select a state representation
#define STATEREP SRCOPY // choose from [state-representation]

#define DOFRECREATE 1
#define DOFUPDATE 2
#define DOFUPDATEWITHREACHABILITY 3

// type of search
#define SEARCHTYPE HEURISTICSEARCH // choose from [search-type]

// options for heuristic search
#define ASTAR
#define GASTARWEIGHT 2

#define PRGEFFECTLESS // always progress effectless actions

#define ONEMODAC
//#define ONEMODMETH
//#define HEURISTIC RCLMC2
//#define RCHEURISTIC2

// configure DOF
//#define HEURISTIC DOFREEILP
#define HEURISTIC DOFREELP

// use TDG constraints
// #define ILPTDG cTdgFull
#define ILPTDG cTdgAllowUC

// use planning graph constraints
#define ILPPG cPgFull
// #define ILPPG cPgTimeRelaxed
// #define ILPPG cPgNone

// use AND/OR landmark constraints
//#define ILPANDORLMS cAndOrLmsNone
//#define ILPANDORLMS cAndOrLmsOnlyTnI
#define ILPANDORLMS cAndOrLmsFull

// use LM-Cut landmark constraints
//#define ILPLMCLMS cLmcLmsNone
#define ILPLMCLMS cLmcLmsFull

// use net change constraints
//#define ILPNC cNetChangeNone
#define ILPNC cNetChangeFull

#define RCLMC2STORELMS

//#define CORRECTTASKCOUNT

#ifdef CORRECTTASKCOUNT
#define CALCMINIMALIMPLIEDCOSTS
#endif

//#define HEURISTIC RCFF
//#define RCHEURISTIC

#define TRACKTASKSINTN

//#define TREATSCCS // prevent disconnected components
#define INITSCCS

//#define DOFTASKREACHABILITY // store the hierarchical task reachability in the ILP to make  it easier to solve
#define DOFREE
#define CHECKAFTER 50 // nodes after which the timelimit is checked
#define MAINTAINREACHABILITY
#define ALLTASKS // it is needed for all tasks

// bis

//#define CHECKAFTER 5000 // nodes after which the timelimit is checked
//#define MAINTAINREACHABILITY
//#define ONLYACTIONS // it is only needed for actions
#ifndef OPTIMIZEUNTILTIMELIMIT
#define OPTIMIZEUNTILTIMELIMIT false
#endif

#ifndef CHECKAFTER
#define CHECKAFTER 5000 // nodes after which the timelimit is checked
#endif


//#define TRACESOLUTION


#endif /* FLAGS_H_ */
