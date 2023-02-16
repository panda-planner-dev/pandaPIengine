#include "translationController.h"
#include "htnToSAS.h"


#include <sys/stat.h>
#include <sys/time.h>


inline bool does_file_exist (const string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}


// returns false if translation failed
bool performOneTranslation(HTNToSASTranslation * translation, TranslationType transtype, int pgb){
	// time measurement	
	timeval tp;
	long startT;
	long currentT;
	gettimeofday(&tp, NULL);
	startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;


	if (transtype == Push){
		int b = translation->htnToCondSorted(pgb);
		cout << "Number of actions: " << b << " pgb: " << pgb << " ";
		if (b == -1) return false;
	}
    
	gettimeofday(&tp, NULL);
    currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << " (" << (currentT - startT) << " ms)" << endl;

	return true;
}

void printSASToFile(HTNToSASTranslation * translation, TranslationType transtype, string sasfile){
	/*
    * Print to file
    */
	timeval tp;
	long startT;
	long currentT;
    gettimeofday(&tp, NULL);
    startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "Printing HTN model to file \"" << sasfile << "\" ... ";
    translation->writeToFastDown(sasfile, transtype == Push);
    gettimeofday(&tp, NULL);
    currentT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << " (" << (currentT - startT) << " ms)" << endl;
}

int runFD(string sasfile, string solver){
	string planFileName = sasfile + string(".plan");
    string command = solver + string(" --internal-plan-file ") + planFileName;
	
	
	int downwardConf = 1;
	switch (downwardConf){
		case 0: command += string(" --evaluator 'hcea=cea()' --search 'lazy_greedy([hcea], preferred=[hcea])' < ") + sasfile; break;
		case 1: command += string(" --evaluator 'hff=ff()' --search 'iterated([ehc(hff, preferred=[hff]),lazy_greedy([hff], preferred=[hff])], continue_on_fail=true, continue_on_solve=false, max_time=1800)' < ") + sasfile; break;
		case 2: command += string(" --portfolio /home/behnkeg/classical/Saarplan/driver/portfolios/seq_agl_saarplan.py ") + sasfile; break;
		case 3: command += string(" --build=release64 --search-time-limit 10000 --search --alias seq-sat-fdss-2018 ") + sasfile; break;
	}


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

void runTranslationPlanner(Model* htn, TranslationType transtype, int pgb, string downward, string sasfile,
		bool iterate,
		bool onlyGenerate){
	int maxpgb = -1;
	int pgbsteps = 1;
	int downwardConf = 0;
	string planfile = "stdout";


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


	while (true){
		// what to do
		performOneTranslation(translation, transtype, pgb);
		printSASToFile(translation, transtype, sasfile);
		if (onlyGenerate) break;

		int error_code = runFD(sasfile,downward);
    	if (error_code == 0 || error_code == 2){
			// found a solution
		}
		if (does_file_exist(planFileName) || does_file_exist(planFileName + string(".1"))) {
		}
	
	
		if (error_code == 3072 || error_code == 2816 || error_code == 512 || error_code == 1280 || error_code > 10){
    	  if (problemType == ParallelSeq){
    	    for (int i = 0; i < parallel; i++){
    	      pgbList[i] += pgbsteps;
    	    }
    	  }
    	  pgb += pgbsteps;
    	  cout << "- new progressionbound: " << pgb << endl;
    	}
    	else {
    	  cout << "- FD produced unknown error code: " << error_code << endl;
    	  exit(-1)
    	}
	
	}

	// we don't correctly clean up after ourselves -- yet
	exit(0);
}

