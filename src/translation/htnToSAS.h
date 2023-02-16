#ifndef HTNTOSAS_H_
#define HTNTOSAS_H_

#include "../Model.h"


class HTNToSASTranslation{
	Model* htn;

	public:
	
	HTNToSASTranslation(Model* model): htn(model){};
	// model preparation
	void reorderTasks(bool warning);
	void sasPlus();
	
	// actual translation
	int htnToCondSorted(int pgb);

	// write the model to file
	void writeToFastDown(string sasName, bool hasCondEff);


	private:
		
	// sasPlus related
	bool* sasPlusBits;
	int* sasPlusOffset;
	int* bitsToSP;
	bool* bitAlone;

	int numStateBitsSP;

	string* factStrsSP;
	int* firstIndexSP;
	int* lastIndexSP;

	int** strictMutexesSP;
	
	int** mutexesSP;

	int** precListsSP;
	int** addListsSP;

	int* numPrecsSP;
	int* numAddsSP;

	int* s0ListSP;
	int* gListSP;




	// For TOHTN
	int** subTasksInOrder;
	int* taskToKill;
	int* firstNumTOPrimTasks;
	
	// state-bits with strips translation
	int numStateBitsTrans;
	string* factStrsTrans;
	// variable definitions for strips translation
	int numVarsTrans;
	int headIndex;
	int firstTaskIndex;
	int firstVarIndex;
	int firstConstraintIndex;
	int firstStackIndex;
	int* firstIndexTrans;
	int* lastIndexTrans;
	string* varNamesTrans;
	
	// for making sure every method has a last task
	bool* hasNoLastTask;
	int numEmptyTasks;
	int firstEmptyTaskIndex;
	string* emptyTaskNames;
	int* numEmptyTaskPrecs;
	int* numEmptyTaskAdds;
	int** emptyTaskPrecs;
	int** emptyTaskAdds;

	// action definitions translation
	int numActionsTrans;
	int numMethodsTrans;
	int firstMethodIndex;
	int* methodIndexes;
	bool* invalidTransActions;
	int numInvalidTransActions;

	string* actionNamesTrans;

	int* actionCostsTrans;
	int** precListsTrans;
	int** addListsTrans;
	int** delListsTrans;

	int* numPrecsTrans;
	int* numAddsTrans;
	int* numDelsTrans;
	
	int* numConditionalEffectsTrans;
	int** numEffectConditionsTrans;
	int*** effectConditionsTrans;
	int** effectsTrans;
	
	// s0 strips translation
	int* s0ListTrans;
	int s0SizeTrans;
	int* gListTrans;
	int gSizeTrans;
		
	
	// progression bound
	int* minImpliedPGB;

	// math helper functions
	int bin(int n, int k);
	int power(int n, int p);
	void combination(int* array, int n, int k, int i);





	void calcMinimalProgressionBound(bool to);
	// translation to strips
	void tohtnToStrips(int pgb);
	int htnToCond(int pgb);
	int htnPS(int numSeq, int* pgbList);
	int htnToStrips(int pgb);
	int minProgressionBound();
	int maxProgressionBound();
	
	void printActionsToFile(string file);
	void printStateBitsToFile(string file);
	bool parallelSequences();
  int calculatePrecsAndAdds(int* s, int* p, int* a, string tasks, int** conv);

  void checkFastDownwardPlan(string domain, string plan);

	// plan verification
	void planToHddl(string infile, string outfile);


};



#endif
