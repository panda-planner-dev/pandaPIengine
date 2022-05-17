//============================================================================
// Name        : SearchEngine.cpp
// Author      : Daniel Höller
// Version     :
// Copyright   : 
// Description : Search Engine for Progression HTN Planning
//============================================================================

#include "flags.h" // defines flags
#include <sys/stat.h>

#include <iostream>
#include <stdlib.h>
#include <cassert>
#include <unordered_set>
#include <sys/time.h>


#include "Model.h"

using namespace std;
using namespace progression;

inline bool does_file_exist (const string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}


int main(int argc, char *argv[]) {  
  string s;
	string sasfile;
	string solver;
	string parserpath = "./pandaPIparser/pandaPIparser";
	bool showUsage = false;
  int pgb = -1;
  int maxpgb = -1;
  int pgbsteps = 1;
  int problemType = 0;
  int downwardConf = 0;
  string planfile = "stdout";
  string domainfile;
  string problemfile;
  if (argc < 4) {
	  showUsage = true;
	} else {
		s = argv[1];
		sasfile = argv[2];
		solver = argv[3];
		for (int i = 4; i < argc; i++){
      if (string("--help").compare(argv[i]) == 0){
        cerr << "showing help" << endl;
        showUsage = true;
        break;
      }
      if (argc == i + 1){
        cerr << argv[i] << ": missing argument" << endl;
        showUsage = true;
        break;
      }
      if (string("--problem").compare(argv[i]) == 0){
        try {
          problemType = stoi(argv[i+1], nullptr);
        }
        catch (...) {
          cerr << "--problem: invalid option: " << argv[i + 1] << endl;
          showUsage = true;
          break;
        }
        if (problemType > 4){
          cerr << "--problem: invalid option: " << problemType << endl;
          showUsage = true;
          break;
        }
      }
      else if (string("--pgb").compare(argv[i]) == 0){
        try {
           pgb = stoi(argv[i+1], nullptr);
        }
        catch (...) {
          cerr << "--pgb: invalid option: " << argv[i + 1] << endl;
          showUsage = true;
          break;
        }
      }
      else if (string("--parserpath").compare(argv[i]) == 0){
        parserpath = string(argv[i+1]);
      }
      else if (string("--domainfile").compare(argv[i]) == 0){
        domainfile = string(argv[i+1]);
      }
      else if (string("--problemfile").compare(argv[i]) == 0){
        problemfile = string(argv[i+1]);
      }
      else if (string("--maxpgb").compare(argv[i]) == 0){
        try {
          maxpgb = stoi(argv[i+1], nullptr);
        }
        catch (...) {
          cerr << "--maxpgb: invalid option: " << argv[i + 1] << endl;
          showUsage = true;
          break;
        }
      }
      else if (string("--pgbsteps").compare(argv[i]) == 0){
        try {
          pgbsteps = stoi(argv[i+1], nullptr);
        }
        catch (...) {
          cerr << "--pgbsteps: invalid option: " << argv[i + 1] << endl;
          showUsage = true;
          break;
        }
      }
      else if (string("--downward").compare(argv[i]) == 0){
        try {
          downwardConf = stoi(argv[i+1], nullptr);
        }
        catch (...) {
          cerr << "--downward: invalid option (must be a number): " << argv[i + 1] << endl;
          showUsage = true;
          break;
        }
      }
      else if (string("--planfile").compare(argv[i]) == 0){
        try {
          new (&planfile) string(argv[i+1]);
        }
        catch (...) {
          cerr << "--downward: invalid option (must be a number): " << argv[i + 1] << endl;
          showUsage = true;
          break;
        }
      }
      else{
        cerr << "did not recognise option: " << argv[i] << endl;
        showUsage = true;
        break;
      }
		  i++;
		}
		
	}
	if (showUsage){
		cout << endl<< "Usage: PandaPIengine <intput_file> <output_file> <path/to/fast-downward> --options" << endl;
		cout << endl<<"<intput_file>: grounded htn problem" << endl;
		cout << "<output_file>: grounded translated pddl problem" << endl;
		cout << "<path/to/fast-downward>: path to fast downward" << endl;
		cout << "  solves <output_file>" << endl;
		cout << "  default solving method: astar(ff())" << endl;
		cout << "--help: show this help" << endl;
		cout << "--problem <tohtn>/<htn>/<striptshtn>" << endl;
		cout << "  0:" << endl;
		cout << "    input is a totally ordered htn problem" << endl;
		cout << "    output uses only strips operators (default)" << endl;
		cout << "  1:" << endl;
		cout << "    input is any htn problem" << endl;
		cout << "    output uses strips with conditional effects" << endl;
		cout << "  2:" << endl;
		cout << "    input is any htn problem" << endl;
		cout << "    output uses only strips operators" << endl;
		cout << "  3:" << endl;
		cout << "    input is a htn problem with parallel sequences" << endl;
		cout << "    output uses only strips operators" << endl;
		cout << "  4:" << endl;
		cout << "    input is any htn problem" << endl;
		cout << "    implements sorted queue with conditional effects" << endl;
		cout << "--pgb <int>: set the starting progressionbound" << endl;
		cout << "  default: automatic" << endl;
		cout << "--maxpgb <int>: set the maximal progressionbound" << endl;
		cout << "  default: " << maxpgb << endl;
		cout << "--pgbsteps <int>: set the progressionbound stepsize" << endl;
		cout << "  default: " << pgbsteps << endl;
		return 0;
	}


  timeval tp;
  long startT;
  long currentT;

  int error_code = -1;
  /*
   * Read model
   */
  Model* htn = new Model();
  
#ifdef CHECKDOWNWARD
  htn->checkFastDownwardPlan("out.sas", "p.sas");
  
  return 0;
#endif  

  cerr << "Reading HTN model from file \"" << s << "\" ... " << endl;
  htn->read(s);
  
  gettimeofday(&tp, NULL);
  startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  cout << "- reordering subtasks";
  
  htn->reorderTasks(problemType == 3);
  
  gettimeofday(&tp, NULL);
  currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  cout << " (" << (currentT - startT) << " ms)" << endl;
  gettimeofday(&tp, NULL);
  startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  cout << "- creating SAS+ vars";
  
  htn->sasPlus();
  
  gettimeofday(&tp, NULL);
  currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  cout << " (" << (currentT - startT) << " ms)" << endl;

  if (problemType == 3 && !htn->parallelSequences()){
    problemType = 2;
  }
  if (pgb < 1 || problemType == 3){
    htn->calcMinimalProgressionBound((problemType == 0));
    pgb = htn->minProgressionBound();
  }
  if (maxpgb < pgb){
    maxpgb = htn->maxProgressionBound();
  }
  int topMethod = 0;
  int parallel = 0;
  if (problemType == 3){
    topMethod = htn->taskToMethods[htn->initialTask][0];
    parallel = htn->numSubTasks[topMethod];
  }
  int* pgbList = new int[parallel];
  if (problemType == 3){
    for (int i = 0; i < parallel; i++){
      pgbList[i] = htn->minImpliedPGB[htn->subTasks[topMethod][0]];
    }
  }
  cerr << "- starting search:" << endl;
  cerr << "- starting Progressionbound = " << pgb << endl;
  cerr << "- maximum Progressionbound = " << maxpgb << endl;
  cerr << "- Progressionbound steps = " << pgbsteps << endl;
  string foundPlanFileName;
  while (true) {
    
    if (pgb > maxpgb){
      cerr << "maximum progressionbound exeeded" << endl;
      cerr << "terminating process" << endl;
      exit(0);
    }
    /*
    * Translate model
    */
    gettimeofday(&tp, NULL);
    startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    if (problemType == 0){
      cerr << "TOHTN to strips";
      htn->tohtnToStrips(pgb);
    }
    else if (problemType == 1){
      cerr << "HTN to strips with conditional effects";
      int b = htn->htnToCond(pgb);
      if (b == -1){
        cerr << endl << "problem too big to solve" << endl;;
        delete[] pgbList;
        return 0;
      }
    }
    else if (problemType == 2){
      cerr << "HTN to strips";
      int b = htn->htnToStrips(pgb);
	  cout << "Number of actions: " << b << " pgb: " << pgb << endl;
      if (b == -1){
        cerr << endl << "problem too big to solve" << endl;;
        delete[] pgbList;
        return 0;
      }
    }
    else if (problemType == 3){
      cerr << "HTN with parallel total order sequences";
      int b = htn->htnPS(parallel, pgbList);
	  cout << "Number of actions: " << b << " pgb: " << pgb << endl;
    }
    else if (problemType == 4){
      cerr << "HTN to strips with conditional effects and sorted queue";
      int b = htn->htnToCondSorted(pgb);
	  cout << "Number of actions: " << b << " pgb: " << pgb << endl;
      if (b == -1){
        cerr << endl << "problem too big to solve" << endl;;
        delete[] pgbList;
        return 0;
      }
    }
    else {
      cerr << "not a valid problem Type" << endl;
      delete[] pgbList;
      return 0;
    }
    gettimeofday(&tp, NULL);
    currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << " (" << (currentT - startT) << " ms)" << endl;

    /*
    * Print to file
    */
    gettimeofday(&tp, NULL);
    startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cerr << "Printing HTN model to file \"" << sasfile << "\" ... ";
    htn->writeToFastDown(sasfile, problemType, pgb);
    gettimeofday(&tp, NULL);
    currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << " (" << (currentT - startT) << " ms)" << endl;
	string planFileName = sasfile + string(".plan");
    string command = solver + string(" --internal-plan-file ") + planFileName;
	switch (downwardConf){
		case 0: command += string(" --evaluator 'hcea=cea()' --search 'lazy_greedy([hcea], preferred=[hcea])' < ") + sasfile; break;
		case 1: command += string(" --evaluator 'hff=ff()' --search 'iterated([ehc(hff, preferred=[hff]),lazy_greedy([hff], preferred=[hff])], continue_on_fail=true, continue_on_solve=false, max_time=1800)' < ") + sasfile; break;
    	case 2: command += string(" --portfolio /home/behnkeg/classical/Saarplan/driver/portfolios/seq_agl_saarplan.py ") + sasfile; break;
    	case 3: command += string(" --build=release64 --search-time-limit 10000 --search --alias seq-sat-fdss-2018 ") + sasfile; break;
    	case 4: command += string(" --alias seq-sat-fd-autotune-1 ") + sasfile; break;
    	case 5: command += string(" --alias seq-sat-fd-autotune-2 ") + sasfile; break;
    	case 6: command += string(" --alias seq-sat-lama-2011 ") + sasfile; break;
    	case 7: command = solver + string(" --build=aidos_ipc --alias seq-unsolvable-aidos-1 --search-time-limit=30m ") + sasfile; planFileName = "sas_plan"; break;
	}
    gettimeofday(&tp, NULL);
    startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cerr << command << endl;
    error_code = system(command.c_str());
    gettimeofday(&tp, NULL);
    currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "Solving Time (" << (currentT - startT) << " ms) with Error Code: " << error_code << endl << endl;
    if (error_code == 0 || error_code == 2){
	  foundPlanFileName = planFileName;
      break;
    }
  	if (does_file_exist(planFileName) || does_file_exist(planFileName + string(".1"))) {
		foundPlanFileName = planFileName;
		break; // if output file is there, we have a plan
	}
 
	
	if (error_code == 3072 || error_code == 2816 || error_code == 512 || error_code == 1280 || error_code > 10){
      if (problemType == 3){
        for (int i = 0; i < parallel; i++){
          pgbList[i] += pgbsteps;
        }
      }
      pgb += pgbsteps;
      cerr << "- new progressionbound: " << pgb << endl;
    }
    else {
      cerr << "- error code: " << error_code << endl;
      delete[] pgbList;
      return 0;
    }
  }
  delete[] pgbList;
  //string infile = sasfile + ".plan";
  if (! does_file_exist(foundPlanFileName))
	  foundPlanFileName = foundPlanFileName + ".1";
  string outfile = planfile;
  htn->planToHddl(foundPlanFileName, outfile);
  
  //string command = "./pandaPIparser/pandaPIparser -c " + outfile + " c-" + outfile;
  //cerr << command << endl;
  //error_code = system(command.c_str());
  //command = "./pandaPIparser/pandaPIparser -v ./models/to/d_00031.hddl ./models/to/p_00031.hddl c-" + outfile;
  //cerr << command << endl;
  //error_code = system(command.c_str());
 
  return 0;
}
