//============================================================================
// Name        : SearchEngine.cpp
// Author      : Daniel Höller
// Version     :
// Copyright   : 
// Description : Search Engine for Progression HTN Planning
//============================================================================

#include "flags.h" // defines flags

#include <iostream>
#include <stdlib.h>
#include <getopt.h>
#include <unordered_set>
#include <landmarks/lmExtraction/LmCausal.h>
#include <landmarks/lmExtraction/LMsInAndOrGraphs.h>
#include <fringes/OneQueueWAStarFringe.h>
#include "./search/PriorityQueueSearch.h"

#include "Debug.h"
#include "Model.h"

#include "interactivePlanner.h"
#include "sat/sat_planner.h"
#include "Invariants.h"

#include "symbolic_search/automaton.h"


#include "intDataStructures/IntPairHeap.h"
#include "intDataStructures/bIntSet.h"

#include "heuristics/rcHeuristics/RCModelFactory.h"
#include "heuristics/landmarks/lmExtraction/LmFdConnector.h"
#include "heuristics/landmarks/hhLMCount.h"
#ifndef CMAKE_NO_ILP
#include "heuristics/dofHeuristics/hhStatisticsCollector.h"
#endif
#include "VisitedList.h"

#include "cmdline.h"

using namespace std;
using namespace progression;

vector<string> parse_list_of_strings(istringstream & ss){
	vector<string> strings;
	while (true){
		if (ss.eof()) break;
		string x; ss >> x;
		strings.push_back(x);
	}

	return strings;
}

vector<string> parse_list_of_strings(string & line){
	istringstream ss (line);
	return parse_list_of_strings(ss);
}


pair<string,map<string,string>> parse_heuristic_with_arguments_from_braced_expression(string str){
	string heuristic = "";
	size_t pos= 0;
	while (pos < str.size() && str[pos] != '('){
   		heuristic += str[pos];
		pos++;
	}
	
	map<string,string> arguments;

   	if (pos != str.size()){
		string argument_string = str.substr(pos+1,str.size() - pos - 2);
		replace(argument_string.begin(), argument_string.end(), ';', ' ');
		vector<string> args = parse_list_of_strings(argument_string);

		int position = 1;
		for (string arg : args){
			replace(arg.begin(), arg.end(), '=', ' ');
			vector<string> argElems = parse_list_of_strings(arg);
			if (argElems.size() == 1)
				arguments["arg" + to_string(position)] = argElems[0];
			else if (argElems.size() == 2)	
				arguments[argElems[0]] = argElems[1];
			else{
				cout << "option " << arg << " has more than one equals sign ..." << endl;
				exit(1);
			}
			position++;
		}
	} // else there are none

	return make_pair(heuristic,arguments);
}


enum planningAlgorithm{
	PROGRESSION,SAT,BDD,INTERACTIVE
};


void speed_test();

int main(int argc, char *argv[]) {
	//speed_test();
	//return 42;
#ifndef NDEBUG
    cout
            << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug."
            << endl;
#endif

	gengetopt_args_info args_info;
	if (cmdline_parser(argc, argv, &args_info) != 0) return 1;

	// set debug mode
	if (args_info.debug_given) setDebugMode(true);

	int seed = args_info.seed_arg; // has default value
	cout << "Random seed: " << seed << endl;
	srand(seed);
    
	int timeL = args_info.timelimit_arg;
    cout << "Time limit: " << timeL << " seconds" << endl;

	// get input files
	std::vector<std::string> inputFiles;
	for ( unsigned i = 0 ; i < args_info.inputs_num ; ++i )
    	inputFiles.push_back(args_info.inputs[i]);

	std::string inputFilename = "-";

	if (inputFiles.size() > 1){
		std::cerr << "You may specify at most one file as input: the SAS+ problem description" << std::endl;
		return 1;
	} else {
		if (inputFiles.size())
			inputFilename = inputFiles[0];
	}

	std::istream * inputStream;
	if (inputFilename == "-") {
		std::cout << "Reading input from standard input." << std::endl;
		inputStream = &std::cin;
	} else {
		std::cout << "Reading input from " << inputFilename << "." << std::endl;

		std::ifstream * fileInput = new std::ifstream(inputFilename);
		if (!fileInput->good())
		{
			std::cerr << "Unable to open input file " << inputFilename << ": " << strerror (errno) << std::endl;
			return 1;
		}

		inputStream = fileInput;
	}

	//

	bool useTaskHash = true;



    /* Read model */
    // todo: the correct value of maintainTaskRechability depends on the heuristic
    eMaintainTaskReachability reachability = mtrALL;
	bool trackContainedTasks = useTaskHash;
    Model* htn = new Model(trackContainedTasks, reachability, true, true);
	htn->filename = inputFilename;
	if (args_info.satmutexes_flag) htn->rintanenInvariants = true;
	htn->read(inputStream);
	assert(htn->isHtnModel);
	searchNode* tnI = htn->prepareTNi(htn);
			
	if (inputFilename != "-") ((ifstream*) inputStream)->close();

	
    if(reachability != mtrNO) {
        htn->calcSCCs();
        htn->calcSCCGraph();

        // add reachability information to initial task network
        htn->updateReachability(tnI);
    }

//    Heuristic *hLMC = new hhLMCount(htn, 0, tnI, lmfFD);

	planningAlgorithm algo = PROGRESSION;
	if (args_info.progression_given) algo = PROGRESSION;
	if (args_info.sat_given) algo = SAT;
	if (args_info.bdd_given) algo = BDD;
	if (args_info.interactive_given) algo = INTERACTIVE;


	if (algo == INTERACTIVE){
		cout << "Selected Planning Algorihtm: interactive";
		interactivePlanner(htn,tnI);
	} else if (algo == PROGRESSION){
		cout << "Selected Planning Algorithm: progression search";
	
		int hLength = args_info.heuristic_given;
		cout << "Parsing heuristics ..." << endl;
		cout << "Number of specified heuristics: " << hLength << endl;
		if (hLength == 0){
			cout << "No heuristics given, setting default ... " << endl;
			hLength = 1;
		}
    	Heuristic **heuristics = new Heuristic *[hLength];
		for (int i = 0; i < hLength; i++){
			auto [hName, args] = parse_heuristic_with_arguments_from_braced_expression(args_info.heuristic_arg[i]);
		
			if (hName == "zero"){
    			heuristics[i] = new hhZero(htn, i);
			} else if (hName == "rc2"){
				string subName = (args.count("h"))?args["h"]:args["arg1"];
			
				string estimate_string = (args.count("est"))?args["est"]:args["arg2"];
				eEstimate estimate = estDISTANCE;
				if (estimate_string == "cost")
					estimate = estCOSTS;
				
				string correct_task_count_string = (args.count("taskcount"))?args["taskcount"]:args["arg3"];
				bool correctTaskCount = true;
				if (correct_task_count_string == "no")
					correctTaskCount = false;

				if (subName == "lmc")
		    		heuristics[i] = new hhRC2<hsLmCut>(htn, i, estimate, correctTaskCount);
				else if (subName == "add"){
		    		heuristics[i] = new hhRC2<hsAddFF>(htn, i, estimate, correctTaskCount);
					((hhRC2<hsAddFF>*)heuristics[i])->sasH->heuristic = sasAdd;
				} else if (subName == "ff"){
		    		heuristics[i] = new hhRC2<hsAddFF>(htn, i, estimate, correctTaskCount);
					((hhRC2<hsAddFF>*)heuristics[i])->sasH->heuristic = sasFF;
				} else if (subName == "filter")
		    		heuristics[i] = new hhRC2<hsFilter>(htn, i, estimate, correctTaskCount);
				else {
					cout << "No inner heuristic specified for RC. Using FF" << endl;
		    		heuristics[i] = new hhRC2<hsAddFF>(htn, i, estimate, correctTaskCount);
					((hhRC2<hsAddFF>*)heuristics[i])->sasH->heuristic = sasFF;
				}
			} else if (hName == "dof"){
#ifndef CMAKE_NO_ILP
				string type_string = (args.count("type"))?args["type"]:args["arg1"];
				IloNumVar::Type intType = IloNumVar::Int;
				IloNumVar::Type boolType = IloNumVar::Bool;
				if (type_string == "lp"){
					intType = IloNumVar::Float;
					boolType = IloNumVar::Float;
				}

				string mode_string = (args.count("mode"))?args["mode"]:args["arg2"];
				csSetting mode = cSatisficing;
				if (mode_string == "optimal")
					mode = cOptimal;

				string tdg_string = (args.count("tdg"))?args["tdg"]:args["arg3"];
				csTdg tdg = cTdgAllowUC;
				if (tdg_string == "uc")
					tdg = cTdgFull;
				else if (tdg_string == "none")
					tdg = cTdgNone;
			
				string pg_string = (args.count("pg"))?args["pg"]:args["arg4"];
				csPg pg = cPgNone;
				if (pg_string == "full")
					pg = cPgFull;
				else if (tdg_string == "relaxed")
					pg = cPgTimeRelaxed;

				string andorLM_string = (args.count("andOrLM"))?args["andOrLM"]:args["arg5"];
				csAndOrLms andOrLM = cAndOrLmsNone;
				if (andorLM_string == "full")
					andOrLM = cAndOrLmsFull;
				else if (andorLM_string == "onlyTNi")
					andOrLM = cAndOrLmsOnlyTnI;

				string externalLM_string = (args.count("externalLM"))?args["externalLM"]:args["arg6"];
				csAddExternalLms externalLM = cAddExternalLmsNo;
				if (externalLM_string == "none")
					externalLM = cAddExternalLmsYes;

				string lmclms_string = (args.count("lmclmc"))?args["lmclmc"]:args["arg7"];
				csLmcLms lmclms = cLmcLmsFull;
				if (lmclms_string == "none")
					lmclms = cLmcLmsNone;

				string netchange_string = (args.count("netchange"))?args["netchange"]:args["arg8"];
				csNetChange netchange = cNetChangeFull;
				if (netchange_string == "none")
					netchange = cNetChangeNone;


				heuristics[i] = new hhDOfree(htn,tnI,i,intType,boolType,mode,tdg,pg,andOrLM,lmclms,netchange,externalLM);
#else
				cout << "Planner compiled without CPLEX support" << endl;
				return 1;
#endif
			} else {
				cout << "Heuristic type \"" << hName << "\" is unknown." << endl;
				return 1;
			}

			cout << "Heuristic #" << i << " = " << heuristics[i]->getDescription() << endl; 
		}

		int aStarWeight = args_info.astarweight_arg;
    	aStar aStarType = gValNone;
    	if (string(args_info.gValue_arg) == "path") aStarType = gValPathCosts;
    	if (string(args_info.gValue_arg) == "action") aStarType = gValActionCosts;
    	if (string(args_info.gValue_arg) == "mixed") aStarType = gValActionPathCosts;
    	if (string(args_info.gValue_arg) == "none") aStarType = gValNone;
		
		bool suboptimalSearch = args_info.suboptimal_flag;

		cout << "Search config:" << endl;
		cout << " - type: ";
		switch (aStarWeight){
			case gValNone: cout << "greedy"; break;
			case gValActionCosts: cout << "cost optimal"; break;
			case gValPathCosts: cout << "path cost"; break;
			case gValActionPathCosts: cout << "action cost + decomposition cost"; break;
		}
		cout << endl;
		cout << " - weight: " << aStarWeight << endl;
		cout << " - suboptimal: " << (suboptimalSearch?"true":"false") << endl;

		
		bool noVisitedList = args_info.noVisitedList_flag;
		bool allowGIcheck = args_info.noGIcheck_flag;
		bool taskHash = args_info.taskHash_flag;
		bool taskSequenceHash = args_info.taskSequenceHash_flag;
		bool topologicalOrdering = args_info.topologicalOrdering_flag;
		bool orderPairsHash = args_info.noOrderPairs_flag;
		bool layerHash = args_info.noLayers_flag;
		bool allowParalleSequencesMode = args_info.noParallelSequences_flag;
    	
		VisitedList visi(htn,noVisitedList, taskHash, taskSequenceHash, topologicalOrdering, orderPairsHash, layerHash, allowGIcheck, allowParalleSequencesMode);
    	PriorityQueueSearch search;
    	OneQueueWAStarFringe fringe(aStarType, aStarWeight, hLength);


		bool printPlan = !args_info.noPlanOutput_flag;
    	search.search(htn, tnI, timeL, suboptimalSearch, printPlan, heuristics, hLength, visi, fringe);
	} else if (algo == SAT){
#ifndef CMAKE_NO_SAT
		bool block_compression = args_info.blockcompression_flag;
		bool sat_mutexes = args_info.satmutexes_flag;

    	sat_pruning pruningMode = SAT_FF;
    	if (string(args_info.pruning_arg) == "none") pruningMode = SAT_NONE;
    	if (string(args_info.pruning_arg) == "ff") pruningMode = SAT_FF;
    	if (string(args_info.pruning_arg) == "h2") pruningMode = SAT_H2;

		extract_invariants_from_parsed_model(htn);
		if (sat_mutexes) compute_Rintanen_Invariants(htn);

		solve_with_sat_planner(htn, block_compression, sat_mutexes, pruningMode);
#else
		cout << "Planner compiled without SAT planner support" << endl;
#endif
	} else if (algo == BDD){
#ifndef CMAKE_NO_BDD
		build_automaton(htn);
#else
		cout << "Planner compiled without symbolic planner support" << endl;
#endif
	}

    delete htn;
    
	
	return 0;
}








/*
    long initO, initN;
    long genO, genN;
    long initEl = 0;
    long genEl;
    long start, end;
    long tlmEl;
    long flmEl = 0;
    long mlmEl = 0;
    long tlmO, flmO, mlmO, tlmN, flmN, mlmN;

    timeval tp;
    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    LmCausal* lmc = new LmCausal(htn);
    lmc->prettyprintAndOrGraph();
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    initN = end - start;

    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    LMsInAndOrGraphs* ao = new LMsInAndOrGraphs(htn);
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    initO = end - start;

    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	lmc->calcLMs(tnI);
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    genN = end - start;

    tlmN = landmark::coutLM(lmc->getLMs(), task, lmc->numLMs);
    flmN = landmark::coutLM(lmc->getLMs(), fact, lmc->numLMs);
    mlmN = landmark::coutLM(lmc->getLMs(), METHOD, lmc->numLMs);

    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    ao->generateAndOrLMs(tnI);
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    genO = end - start;

    tlmO = landmark::coutLM(ao->getLMs(), task, ao->getNumLMs());
    flmO = landmark::coutLM(ao->getLMs(), fact, ao->getNumLMs());
    mlmO = landmark::coutLM(ao->getLMs(), METHOD, ao->getNumLMs());

    if(lmc->numLMs != ao->getNumLMs()) {
        cout << "AAAAAAAAAAAAAAAAAAAAHHH " << ao->getNumLMs() << " - " << lmc->numLMs << endl;
        for(int i = 0; i < ao->getNumLMs(); i ++) {
            ao->getLMs()[i]->printLM();
        }
        cout << "----------------------" << endl;
        for(int i = 0; i < lmc->numLMs; i++) {
            lmc->landmarks[i]->printLM();
        }
    }

    cout << "TIME:" << endl;
    cout << "Init       : " << initO << " " << initN << " delta " << (initN - initO) << endl;
    cout << "Generation : " << genO << " " << genN << " delta " << (genN - genO) << endl;
    cout << "Total      : " << (initO + genO) << " " << (initN + genN) << " delta " << ((initN + genN) - (initO + genO)) << endl;

    gettimeofday(&tp, NULL);
    start = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    ao->generateLocalLMs(htn, tnI);
    gettimeofday(&tp, NULL);
    end = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    genEl = end - start;

    tlmEl = landmark::coutLM(ao->getLMs(), task, ao->getNumLMs());
    flmEl = landmark::coutLM(ao->getLMs(), fact, ao->getNumLMs());
    mlmEl = landmark::coutLM(ao->getLMs(), METHOD, ao->getNumLMs());

    cout << "LMINFO:[" << s << ";";
    cout << initEl << ";" << genEl << ";" << (initEl + genEl) << ";";
    cout << initO << ";" << genO << ";" << (initO + genO) << ";";
    cout << initN << ";" << genN << ";" << (initN + genN) << ";";
    cout << tlmEl << ";" << flmEl << ";" << mlmEl << ";";
    cout << tlmO << ";" << flmO << ";" << mlmO << ";";
    cout << tlmN << ";" << flmN << ";" << mlmN << ";";
    cout << "]" << endl;

	//lmc->prettyprintAndOrGraph();
    for(int i = 0; i < htn->numTasks; i++)
        cout << i << " " << htn->taskNames[i] << endl;
    for(int i = 0; i < htn->numStateBits; i++)
        cout << i << " " << htn->factStrs[i] << endl;

    cout << "AND/OR landmarks" << endl;
    for(int i = 0; i < lmc->numLMs; i++) {
        lmc->getLMs()[i]->printLM();
    }
    cout << "Local landmarks" << endl;
    for(int i = 0; i < ao->getNumLMs(); i++) {
       ao->getLMs()[i]->printLM();
    }

    cout << "PRINT" << endl;
    lmc->prettyPrintLMs();

	exit(17);*/

