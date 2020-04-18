
#define HEURISTIC LMCLOCAL

#if ((HEURISTIC == LMCLOCAL) || (HEURISTIC == LMCANDOR))
#define TRACKLMSFULL // track landmarks in the search nodes
#define LMCOUNTHEURISTIC
#define TRACKTASKSINTN
#endif

#ifdef LMCANDORRA
#define MAINTAINREACHABILITY
#define ALLTASKS
#endif


#define CHECKAFTER 5000 // nodes after which the timelimit is checked
