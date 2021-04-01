/*
 * flags.h
 *
 *  Created on: 26.09.2017
 *      Author: Daniel Höller
 */

#ifndef FLAGS_H_
#define FLAGS_H_

typedef int tHVal;
#define tHValUNR INT_MAX

// constants
#define UNREACHABLE INT_MAX
#define NOACTION -1
#define FORBIDDEN -2

// [heuristics]
#define LMCLOCAL 7
#define LMCANDOR 8
#define LMCFD 9

// [state-representation]
#define SRCOPY 0  // copy bit vector that represents state
#define SRCALC1 1 // calculate state based on current plan
#define SRCALC2 2 // calculate state based on current plan
#define SRLIST 3  // maintain int list with bits currently set

// *****************
// * Configuration *
// *****************

// don't perform tests
#define NTEST

// select a state representation
#define STATEREP SRCOPY // choose from [state-representation]


// the following options are preliminary and not part of final version -> can be kept as compiler flags until they are finished
//#define ONEMODMETH // todo: this was always buggy, but the case is compiled away anyway

// todo: LM tracking needs to be extended to LM orderings
//#define TRACKLMSFULL
//#define TRACKLMS




//#define TREATSCCS // prevent disconnected components
#define INITSCCS

#define DOFTASKREACHABILITY // store the hierarchical task reachability in the ILP to make  it easier to solve
//#define CHECKAFTER 50 // nodes after which the timelimit is checked
//#define MAINTAINREACHABILITY
//#define ALLTASKS // it is needed for all tasks

#define CHECKAFTER 5000 // nodes after which the timelimit is checked
#define MAINTAINREACHABILITY
#define ONLYACTIONS // it is only needed for actions
#ifndef OPTIMIZEUNTILTIMELIMIT
#define OPTIMIZEUNTILTIMELIMIT false
#endif

#ifndef CHECKAFTER
#define CHECKAFTER 5000 // nodes after which the timelimit is checked
#endif

#define RINTANEN_INVARIANTS 1


#define TRACESOLUTION
//#define SAVESEARCHSPACE



//#define VISITEDONLYSTATISTICS
#define TOVISI_SEQ  1
#define TOVISI_PRIM 2
#define TOVISI_PRIM_EXACT 3
#define TOVISI TOVISI_PRIM_EXACT

//#define NOVISI



#define POVISI_EXACT
#define POVISI_HASH
#define POVISI_LAYERS
#define POVISI_ORDERPAIRS



// if we write the state space to file, we need to disable pretty much all optimisations ...
#ifdef SAVESEARCHSPACE
#undef OPTIMIZEUNTILTIMELIMIT
#undef PRGEFFECTLESS // todo: this flag is not there anymore! needs to be set in the model constructor
#undef ONEMODAC // todo: this flag is not there anymore! needs to be set in the model constructor
#define OPTIMIZEUNTILTIMELIMIT true
#endif


#endif /* FLAGS_H_ */
