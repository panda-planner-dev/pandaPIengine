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
#include <unordered_set>
#include <landmarks/lmExtraction/LmCausal.h>
#include <landmarks/lmExtraction/LMsInAndOrGraphs.h>

#if SEARCHTYPE == HEURISTICSEARCH
#include "./search/PriorityQueueSearch.h"
#endif

#include "Model.h"
#ifdef RCHEURISTIC
#include "heuristics/rcHeuristics/hsAddFF.h"
#include "heuristics/rcHeuristics/hsLmCut.h"
#include "heuristics/rcHeuristics/hsFilter.h"
#endif
#ifdef RCHEURISTIC2
#include "heuristics/rcHeuristics/hsAddFF.h"
#include "heuristics/rcHeuristics/hsLmCut.h"
#include "heuristics/rcHeuristics/hsFilter.h"
#include "heuristics/rcHeuristics/hhRC2.h"
#endif
#include "intDataStructures/IntPairHeap.h"
#include "intDataStructures/bIntSet.h"

#include "heuristics/rcHeuristics/RCModelFactory.h"
#include "heuristics/landmarks/lmExtraction/LmFdConnector.h"
#include "heuristics/landmarks/hhLMCount.h"

using namespace std;
using namespace progression;

int main(int argc, char *argv[]) {
#ifndef NDEBUG
	cout << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug." << endl;
#endif
	srand(42);
	//srand(atoi(argv[4]));

	string s, s2, s3;
	if (argc == 1) {
		cout << "No file name passed. Reading input from stdin";
		s = "stdin";
		s2 = s3 = "";
	} else {
		s = argv[1];
		s2 = (argc > 2) ? argv[2] : "";
		s3 = (argc > 3) ? argv[3] : "";
	}


/*
 * Read model
 */
	cout << "Reading HTN model from file \"" << s << "\" ... " << endl;
	Model* htn = new Model();
	htn->filename = s;
	htn->read(s);
    //string dOut = "/media/dh/Volume/repositories/neue-repos/publications/2020-HTNWS-Landmarks/stuff/exampleproblems/transport/tranport01-unpruned-domain.hddl";
    //string pOut = "/media/dh/Volume/repositories/neue-repos/publications/2020-HTNWS-Landmarks/stuff/exampleproblems/transport/tranport01-unpruned-problem.hddl";
	//htn->writeToPDDL(dOut,pOut);
	//assert(htn->isHtnModel);
	searchNode* tnI = htn->prepareTNi(htn);

#ifdef MAINTAINREACHABILITY
	htn->calcSCCs();
	htn->calcSCCGraph();

	// add reachability information to initial task network
	htn->updateReachability(tnI);
#endif
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
 * Create Search
 */
#if SEARCHTYPE == HEURISTICSEARCH
	PriorityQueueSearch search;
#endif

/*
 * Create Heuristic
 */
#if HEURISTIC == ZERO
	cout << "Heuristic: 0 for every node" << endl;
	search.hF = new hhZero(htn);
#endif
#ifdef RCHEURISTIC
    cout << "Heuristic: RC encoding" << endl;
	Model* heuristicModel;
	if (s2.size() == 0){
		cout << "No RC model specified. Computing ... " << endl;
		RCModelFactory* factory = new RCModelFactory(htn);
		heuristicModel = factory->getRCmodelSTRIPS();
		delete factory;
	} else {
		heuristicModel = new Model();
		heuristicModel->read(s2);
	}

#if HEURISTIC == RCFF
	search.hF = new hhRC(htn, new hsAddFF(heuristicModel));
	search.hF->sasH->heuristic = sasFF;
	cout << "- Inner heuristic: FF" << endl;
#elif HEURISTIC == RCADD
	search.hF = new hhRC(htn, new hsAddFF(heuristicModel));
	search.hF->sasH->heuristic = sasAdd;
	cout << "- Inner heuristic: Add" << endl;
#elif HEURISTIC == RCLMC
	search.hF = new hhRC(htn, new hsLmCut(heuristicModel));
	cout << "- Inner heuristic: LM-Cut" << endl;
#elif HEURISTIC == RCFILTER
	search.hF = new hhRC(htn, new hsFilter(heuristicModel));
	cout << "- Inner heuristic: Filter" << endl;
#endif
#endif
#ifdef RCHEURISTIC2
    cout << "Heuristic: RC encoding" << endl;

#if HEURISTIC == RCFF2
	search.hF = new hhRC(htn, new hsAddFF(heuristicModel));
	search.hF->sasH->heuristic = sasFF;
	cout << "- Inner heuristic: FF" << endl;
#elif HEURISTIC == RCADD2
	search.hF = new hhRC(htn, new hsAddFF(heuristicModel));
	search.hF->sasH->heuristic = sasAdd;
	cout << "- Inner heuristic: Add" << endl;
#elif HEURISTIC == RCLMC2
	search.hF = new hhRC2(htn);
	cout << "- Inner heuristic: LM-Cut" << endl;
#elif HEURISTIC == RCFILTER2
	search.hF = new hhRC(htn, new hsFilter(heuristicModel));
	cout << "- Inner heuristic: Filter" << endl;
#endif
#endif
#ifdef DOFREE
    search.hF = new hhDOfree(htn, tnI, IloNumVar::Float, IloNumVar::Float, cTdgAllowUC, cPgFull, cAndOrLmsOnlyTnI, cLmcLmsFull, cNetChangeFull, cAddExternalLmsNo);
    //search.hF = new hhDOfree<IloNumVar::Int, IloNumVar::Bool>(htn, tnI);
#endif
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
	int timeL = TIMELIMIT;
	cout << "Time limit: " << timeL << " seconds" << endl;
	search.search(htn, tnI, timeL);
	delete htn;
#ifdef RCHEURISTIC
	delete heuristicModel;
#endif
	return 0;
}
