#include "translationController.h"
#include "htnToSAS.h"


#include <cassert>
#include <cstdio>
#include <sys/stat.h>
#include <sys/time.h>


inline bool does_file_exist (const string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}


// returns false if translation failed
bool performOneTranslation(HTNToSASTranslation * translation, TranslationType transtype, int parallel, int* pgbList){
	// time measurement	
	timeval tp;
	long startT;
	long currentT;
	gettimeofday(&tp, NULL);
	startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;


	int operators;
	if (transtype == Push){
		cout << endl << "Generating with Push encoding" << endl;
		operators = translation->htnToCondSorted(pgbList[0]);
	} else if (transtype == TO){
		cout << endl << "Generating with TO encoding" << endl;
		operators = translation->tohtnToStrips(pgbList[0]);
	} else if (transtype == ParallelSeq){
		cout << endl << "Generating with Parallel Sequences encoding" << endl;
		operators = translation->htnPS(parallel,pgbList);
	} else if (transtype == BaseStrips){
		cout << endl << "Generating with Basic PO Strips encoding" << endl;
		operators = translation->htnToStrips(pgbList[0]);
	} else if (transtype == BaseCondEffects){
		cout << endl << "Generating with Basic PO ADL/Conditional effects encoding" << endl;
		operators = translation->htnToCondSorted(pgbList[0]);
	} else {
		// impossible to reach
		assert(false);
		exit(-1);
	}
	
	cout << "Number of actions: " << operators << " pgb:";
    for (int i = 0; i < parallel; i++)
		cout << " " << pgbList[i];
	
	if (operators == -1) return false;
    
	gettimeofday(&tp, NULL);
    currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << " (" << (currentT - startT) << " ms)" << endl;

	return true;
}

void printSASToFile(HTNToSASTranslation * translation, TranslationType transtype, string sasfile, bool realCosts){
	/*
    * Print to file
    */
	timeval tp;
	long startT;
	long currentT;
    gettimeofday(&tp, NULL);
    startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "Printing HTN model to file \"" << sasfile << "\" ... ";
    translation->writeToFastDown(sasfile, transtype == Push, realCosts);
    gettimeofday(&tp, NULL);
    currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << " (" << (currentT - startT) << " ms)" << endl;
}

int runFD(string sasfile, string solver, string downwardConf, string & planFileName){
	planFileName = sasfile + string(".plan");
    string command = solver + string(" --internal-plan-file ") + planFileName;
	
	if (downwardConf == "lazy-cea()")
		command += string(" --evaluator 'hcea=cea()' --search 'lazy_greedy([hcea], preferred=[hcea])'");
	else if (downwardConf == "ehc-ff()")
		command += string(" --evaluator 'hff=ff()' --search 'iterated([ehc(hff, preferred=[hff]),lazy_greedy([hff], preferred=[hff])], continue_on_fail=true, continue_on_solve=false, max_time=1800)'");
	else
		command += downwardConf;

	// add the file input
	command += string("  < ") + sasfile;

	// remove plan file before planning
	if (does_file_exist(planFileName)){
		cout << "- old plan file present. Deleting it" << endl;
		remove(planFileName.c_str());
	}
	if (does_file_exist(planFileName + string(".1"))) {
		cout << "- old plan file present. Deleting it" << endl;
		remove((planFileName + string(".1")).c_str());
	}




	// actually strart FD
	timeval tp;
	long startT;
	long currentT;
	gettimeofday(&tp, NULL);
    startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << command << endl;
    int error_code = system(command.c_str());
    gettimeofday(&tp, NULL);
    currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "Solving Time (" << (currentT - startT) << " ms) with Error Code: " << error_code << endl << endl;

	return error_code;
}

void runTranslationPlanner(Model* htn, TranslationType transtype, bool forceTransType,
		int pgb, int pgbsteps, string downward, string downwardConf, string sasfile,
		bool iterate,
		bool onlyGenerate,
		bool realCosts){
	
	
	// overriding of type
	if (!forceTransType){
		if (htn->isTotallyOrdered){
			if (transtype != TO){
				cout << "- Overriding transtype to TO" << endl;
				transtype = TO;
			}
			// else we are ok
		} else {
			if (transtype == TO){
				cout << "- Configuration tried to apply TO encoding to non TO problem ... don't do this. Reverting to Push" << endl;
				transtype = Push;
			}
			if (htn->isParallelSequences){
				if (transtype != ParallelSeq){
					cout << "- Overriding transtype to ParallelSeq" << endl;
					transtype = ParallelSeq;
				}
			} else {
				if (transtype == ParallelSeq){
					cout << "- Configuration tried to apply PSeq encoding to non PSeq problem ... don't do this. Reverting to Push" << endl;
					transtype = Push;
				}
			}

			// Note: we don't override the base encodings. If you want to use them feel free.
		}
	}



	HTNToSASTranslation* translation = new HTNToSASTranslation(htn);
	// prepare the model
	timeval tp;
	long startT;
	long currentT;
	gettimeofday(&tp, NULL);
	startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	cout << "- reordering subtasks";
	//
	translation->reorderTasks(transtype == ParallelSeq);
	//
	gettimeofday(&tp, NULL);
	currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	cout << " (" << (currentT - startT) << " ms)" << endl;



	gettimeofday(&tp, NULL);
	startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	cout << "- creating SAS+ vars";
	//
	translation->sasPlus();
	//
	gettimeofday(&tp, NULL);
	currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	cout << " (" << (currentT - startT) << " ms)" << endl;



	// pgb calculation
	if (pgb <= 0){
		translation->calcMinimalProgressionBound(transtype == TO);
	}

	int parallel = 1;
	int* pgbList;
	if (transtype == ParallelSeq){
		int topMethod = htn->taskToMethods[htn->initialTask][0];
		parallel = htn->numSubTasks[topMethod];
		// generate pgb array
		pgbList = new int[parallel];
		for (int i = 0; i < parallel; i++){
			if (pgb > 0)
				pgbList[i] = pgb;
			else
				pgbList[i] = htn->numSubTasks[topMethod];
		}
	} else {
		pgbList = new int[1];
		if (pgb > 0)
			pgbList[0] = pgb;
		else {
			pgbList[0] = translation->minProgressionBound();
			cout << "- minimum progression bound is: " << pgbList[0] << endl;
		}
	}

	// this is currently just a static high value
	int maxpgb = translation->maxProgressionBound();

	string planFileName; // will be returned by runFD
	while (true){
		// what to do
		performOneTranslation(translation, transtype, parallel, pgbList);
		printSASToFile(translation, transtype, sasfile, realCosts);
		if (onlyGenerate) break;


		int error_code = runFD(sasfile,downward,downwardConf,planFileName);
    	if (does_file_exist(planFileName)){
			// found a solution
			cout << "FD found a solution" << endl;
			break;
		}
		if (does_file_exist(planFileName + string(".1"))) {
			planFileName = planFileName + string(".1");
			cout << "FD found a solution" << endl;
			break;
		}
	
		if (error_code == 3072 || error_code == 2816 || error_code == 512 || error_code == 1280 || error_code > 10){
			if (!iterate){
    			cout << "- Configuration states not to iterate. Stopping. In order to iterate over the bound use --iterate" << endl;
				exit(1);
			}
    		
			for (int i = 0; i < parallel; i++){
    			pgbList[i] += pgbsteps;
				if (pgbList[i] > maxpgb){
		  			cout << "Reached max PGB. Aborting.";
    	  			exit(-1);
				}
			}
    	  
			cout << "- new progressionbound:";
    		for (int i = 0; i < parallel; i++)
				cout << " " << pgbList[i];
			cout << endl;
    	} else {
    		cout << "- FD produced unknown error code: " << error_code << endl;
    		exit(-1);
    	}
	}

	translation->planToHddl(planFileName, "stdout");

	// we don't correctly clean up after ourselves -- yet
	exit(0);
}

