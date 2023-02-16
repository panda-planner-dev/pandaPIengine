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

// florian
#define PROGRESSIONBOUND 4
#define CALCMINIMALIMPLIEDCOSTS
// constants
#define UNREACHABLE INT_MAX
#define NOACTION -1
#define FORBIDDEN -2

// [heuristics]
#define LMCLOCAL 7
#define LMCANDOR 8
#define LMCFD 9



// don't perform tests
#define NTEST

// the following options are preliminary and not part of final version -> can be kept as compiler flags until they are finished
//#define ONEMODMETH // todo: this was always buggy, but the case is compiled away anyway

// todo: LM tracking needs to be extended to LM orderings
//#define TRACKLMSFULL
//#define TRACKLMS

// todo switch new implementation second
//RCHEURISTIC


// The following are used by  the members of the search class. Since I do not consider them a parameter frequently
// changed by the users, I am fine with keeping them here, to be changed only in special circumstances.
#define CHECKAFTER 5000 // nodes after which the timelimit is checked
#define OPTIMIZEUNTILTIMELIMIT false // search for more than one solution



#define TRACESOLUTION


// if we write the state space to file, we need to disable pretty much all optimisations ...
#ifdef SAVESEARCHSPACE
#undef OPTIMIZEUNTILTIMELIMIT
#undef PRGEFFECTLESS // todo: this flag is not there anymore! needs to be set in the model constructor
#undef ONEMODAC // todo: this flag is not there anymore! needs to be set in the model constructor
#define OPTIMIZEUNTILTIMELIMIT true
#endif


#endif /* FLAGS_H_ */
