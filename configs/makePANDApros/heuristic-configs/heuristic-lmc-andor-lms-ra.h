
#define HEURISTIC LMCANDOR
#define LMCANDORRA // flag for reachability analysis

#if ((HEURISTIC == LMCLOCAL) || (HEURISTIC == LMCANDOR))
#define TRACKLMS // track landmarks in the search nodes
#endif

#ifdef LMCANDORRA
#define MAINTAINREACHABILITY
#define ALLTASKS
#endif

#define CHECKAFTER 5000 // nodes after which the timelimit is checked
