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
        if (problemType > 2){
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
		cout << "--pgb <int>: set the starting progressionbound" << endl;
		cout << "  default: automatic" << endl;
		cout << "--maxpgb <int>: set the maximal progressionbound" << endl;
		cout << "  default: " << maxpgb << endl;
		cout << "--pgbsteps <int>: set the progressionbound stepsize" << endl;
		cout << "  default: " << pgbsteps << endl;
		return 0;
	}


  int error_code = -1;
  /*
   * Read model
   */
  cerr << "Reading HTN model from file \"" << s << "\" ... " << endl;
  Model* htn = new Model();
  htn->read(s);
	//htn->printStateBitsToFile(string("stateBits1.txt"));
  //htn->printActionsToFile(string("actions1.txt"));
  htn->sasPlus();
  //htn->printActionsToFile(string("actions2.txt"));
	//htn->printStateBitsToFile(string("stateBits2.txt"));
  if (pgb < 1){
    pgb = htn->minProgressionBound();
  }
  if (maxpgb < pgb){
    maxpgb = htn->maxProgressionBound();
  }
  cerr << "starting search:" << endl;
  cerr << "starting Progressionbound = " << pgb << endl;
  cerr << "maximum Progressionbound = " << maxpgb << endl;
  cerr << "Progressionbound steps = " << pgbsteps << endl;
  while (true) {
    
    if (pgb > maxpgb){
      cerr << "maximum progressionbound exeeded" << endl;
      cerr << "terminating process" << endl;
      break;
    }
    /*
    * Translate model
    */
    if (problemType == 0){
      cerr << "totally ordered htn to Strips" << endl;
      htn->tohtnToStrips(pgb);
    }
    else if (problemType == 1){
      cerr << "htn to Strips with conditional effects" << endl;
      htn->htnToCond(pgb);
    }
    else{
      cerr << "htn to strips" << endl;
      htn->htnToStrips(pgb);
    }
    
    /*
    * Print to file
    */
    cerr << "Printing HTN model to file \"" << sasfile << "\" ... " << endl;
    htn->writeToFastDown(sasfile, problemType, pgb);
    string command;
	switch (downwardConf){
		case 0: command = solver + string(" --evaluator 'hcea=cea()' --search 'lazy_greedy([hcea], preferred=[hcea])' --internal-plan-file ") + sasfile + string(".plan < ") + sasfile; break;
		case 1: command = solver + string(" --evaluator 'hff=ff()' --search 'iterated([ehc(hff, preferred=[hff]),lazy_greedy([hff], preferred=[hff])], continue_on_fail=true, continue_on_solve=false, max_time=1800)' --internal-plan-file ") + sasfile + string(".plan < ") + sasfile; break;
    case 2: command = solver + ' ' + sasfile + string(" --evaluator 'hcea=cea()' --search 'lazy_greedy([hcea], preferred=[hcea])'"); break;
	}
    cerr << command << endl;
    error_code = system(command.c_str());
    if (error_code == 0){
      break;
    }
    else if (error_code == 3072){
      pgb += pgbsteps;
      cerr << "new progressionbound: " << pgb << endl;
    }
    else {
      cerr << "error code: " << error_code << endl;
      return 0;
    }
  }
  string infile = sasfile + ".plan";
  if (! does_file_exist(infile))
	  infile = infile + ".1";
  string outfile = planfile;
  htn->planToHddl(infile, outfile);
  
  //string command = "./pandaPIparser/pandaPIparser -c " + outfile + " c-" + outfile;
  //cerr << command << endl;
  //error_code = system(command.c_str());
  //command = "./pandaPIparser/pandaPIparser -v ./models/to/d_00031.hddl ./models/to/p_00031.hddl c-" + outfile;
  //cerr << command << endl;
  //error_code = system(command.c_str());
 
  return 0;
}
