
// select a heuristic function
#define HEURISTIC DOFREEILP

#define DOFTR  // time relaxation proposed in the paper, i.e. omitting C5 and C6

#define DOFTASKREACHABILITY // store the hierarchical task reachability in the ILP to make  it easier to solve
#define DOFREE
#define CHECKAFTER 50 // nodes after which the timelimit is checked
#define MAINTAINREACHABILITY
#define ALLTASKS // it is needed for all tasks
