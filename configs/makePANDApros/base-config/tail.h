#ifndef OPTIMIZEUNTILTIMELIMIT
#define OPTIMIZEUNTILTIMELIMIT false
#endif

#if (HEURISTIC == DOFREEILP || HEURISTIC == DOFREELP)
#define DOFREE
#endif

#ifdef DOFREE
#define CHECKAFTER 5
#define MAINTAINREACHABILITY
#define ALLTASKS // it is needed for all tasks
#define TRACKTASKSINTN
#endif

#ifndef CHECKAFTER
#define CHECKAFTER 5000 // nodes after which the timelimit is checked
#endif

#endif /* FLAGS_H_ */
