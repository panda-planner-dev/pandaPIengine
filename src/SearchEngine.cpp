//============================================================================
// Name        : SearchEngine.cpp
// Author      : Daniel Höller
// Version     :
// Copyright   : 
// Description : Search Engine for Progression HTN Planning
//============================================================================

#include "flags.h" // defines flags

#include <queue>
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

#include "heuristics/rcHeuristics/hsAddFF.h"
#include "heuristics/rcHeuristics/hsLmCut.h"
#include "heuristics/rcHeuristics/hsFilter.h"
#include "heuristics/rcHeuristics/hhRC2.h"

#include "intDataStructures/IntPairHeap.h"
#include "intDataStructures/bIntSet.h"

#include "heuristics/rcHeuristics/RCModelFactory.h"
#include "heuristics/landmarks/lmExtraction/LmFdConnector.h"
#include "heuristics/landmarks/hhLMCount.h"
#include "heuristics/dofHeuristics/hhStatisticsCollector.h"
#include "VisitedList.h"

#include "cmdline.h"

using namespace std;
using namespace progression;

int main(int argc, char *argv[]) {
#ifndef NDEBUG
    cout
            << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug."
            << endl;
#endif

	gengetopt_args_info args_info;
	if (cmdline_parser(argc, argv, &args_info) != 0) return 1;

	// set debug mode
	if (args_info.debug_given) setDebugMode(true);

	int seed = 42;
	if (args_info.seed_given) seed = args_info.seed_arg;
	cout << "Random seed: " << seed << endl;
	srand(seed);

	// FILES


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


    /* Read model */
	Model* htn = new Model(false, true, true);
	htn->filename = inputFilename;
	htn->read(inputStream);
	assert(htn->isHtnModel);
	searchNode* tnI = htn->prepareTNi(htn);

	if (inputFilename != "-") ((ifstream*) inputStream)->close();



#ifdef MAINTAINREACHABILITY
    htn->calcSCCs();
    htn->calcSCCGraph();

    // add reachability information to initial task network
    htn->updateReachability(tnI);
#endif
/*
	cout << "YouPlan!" << endl;
	while(true) {
	    int j = 0;
        for(int i = 0; i < tnI->numAbstract; i++){
            cout << j++ << " " << htn->taskNames[tnI->unconstraintAbstract[i]->task] << endl;
        }
        for(int i = 0; i < tnI->numPrimitive; i++){
            cout << j++ << " " << htn->taskNames[tnI->unconstraintPrimitive[i]->task] << endl;
        }
        int step;
	    cout << "What to do?" << endl;
	    if (j == 1)
	        step = 0;
	    else
            cin >> step;
        if(step == -2) break;
        if(step == -1) exit(0);
        if(step < tnI->numAbstract) {
            int t = tnI->unconstraintAbstract[step]->task;
            int i = 0;
            for(; i < htn->numMethodsForTask[t]; i++) {
                int m = htn->taskToMethods[t][i];
                cout << i << " " << htn->methodNames[m] << endl;
            }
            cout << "Which method to use?" << endl;
            int step2;
            if (i == 1)
                step2 = 0;
            else
                cin >> step2;
            int m = htn->taskToMethods[t][step2];
            tnI = htn->decompose(tnI, step, m);
        } else {
            step -= tnI->numAbstract;
            int a = tnI->unconstraintPrimitive[step]->task;
            cout << "prec:" << endl;
            for(int i = 0; i < htn->numPrecs[a]; i++) {
                int f = htn->precLists[a][i];
                cout << "- " << f << " " << htn->factStrs[f] << endl;
            }
            cout << "add:" << endl;
            for(int i = 0; i < htn->numAdds[a]; i++) {
                int f = htn->addLists[a][i];
                cout << "- " << f << " " << htn->factStrs[f] << endl;
            }
            cout << "del:" << endl;
            for(int i = 0; i < htn->numDels[a]; i++) {
                int f = htn->delLists[a][i];
                cout << "- " << f << " " << htn->factStrs[f] << endl;
            }
            tnI = htn->apply(tnI, step);
        }
	}
*/
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

/*
 * Create Heuristic
 */
#if (HEURISTIC == LMCLOCAL)
    search.hF = new hhLMCount(htn, tnI, 0);
    search.hF->prettyPrintLMs(htn, tnI);
#endif
#if (HEURISTIC == LMCANDOR)
    search.hF = new hhLMCount(htn, tnI, 1);
    search.hF->prettyPrintLMs(htn, tnI);
#endif
#if (HEURISTIC == LMCFD)
    search.hF = new hhLMCount(htn, tnI, 2);
    search.hF->prettyPrintLMs(htn, tnI);
#endif
/*
 * Start Search
 */
    int timeL = 1800;
    cout << "Time limit: " << timeL << " seconds" << endl;

    int hLength = 5;
    Heuristic **heuristics = new Heuristic *[hLength];
    heuristics[0] = new hhRC2<hsLmCut>(htn, 0, estDISTANCE, true);

    // old RC heuristic implementation
    Model* heuristicModel;
    cout << "Computing RC model ... " << endl;
    RCModelFactory* factory = new RCModelFactory(htn);
    heuristicModel = factory->getRCmodelSTRIPS();
    delete factory;

    auto oldRC = new hhRC<hsAddFF>(htn, heuristicModel, 1, estDISTANCE, false);
    oldRC->sasH->heuristic = sasFF;
    heuristics[1] = oldRC;

    oldRC = new hhRC<hsAddFF>(htn, heuristicModel, 2, estDISTANCE, false);
    oldRC->sasH->heuristic = sasAdd;
    heuristics[2] = oldRC;

    heuristics[3] = new hhRC<hsLmCut>(htn, heuristicModel, 2, estDISTANCE, false);
    heuristics[4] = new hhRC<hsFilter>(htn, heuristicModel, 2, estDISTANCE, false);

//    hhRC2<hsAddFF> *hF2 = new hhRC2<hsAddFF>(htn, 1, ctcINADMISSIBLE);
//    hF2->sasH->heuristic = sasAdd;
//    heuristics[1] = hF2;
//
//    hhRC2<hsAddFF> *hF3 = new hhRC2<hsAddFF>(htn, 2, ctcINADMISSIBLE);
//    hF2->sasH->heuristic = sasFF;
//    heuristics[2] = hF3;
//
//    heuristics[3] = new hhRC2<hsFilter>(htn, 3, ctcINADMISSIBLE);
//
//    heuristics[4] = new hhDOfree(htn, tnI, 4, IloNumVar::Int, IloNumVar::Bool, cSatisficing, cTdgAllowUC, cPgNone,
//                                 cAndOrLmsNone, cLmcLmsFull, cNetChangeFull, cAddExternalLmsNo);
//
//    heuristics[5] = new hhDOfree(htn, tnI, 5, IloNumVar::Float, IloNumVar::Float, cSatisficing, cTdgAllowUC, cPgNone,
//                                 cAndOrLmsNone, cLmcLmsFull, cNetChangeFull, cAddExternalLmsNo);

    // heuristics[6] = new hhZero(htn, 6);

    int aStarWeight = 1;
    aStar aStarType = gValNone;
    bool suboptimalSearch = false;

    VisitedList visi(htn);
    PriorityQueueSearch search;
    OneQueueWAStarFringe fringe(aStarType, aStarWeight, hLength);

    search.search(htn, tnI, timeL, suboptimalSearch, heuristics, hLength, visi, fringe);
    delete htn;
#ifdef RCHEURISTIC
    delete heuristicModel;
#endif
    return 0;
}
