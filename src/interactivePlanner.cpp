#include "interactivePlanner.h"

#include <queue>
#include <iostream>


void interactivePlanner(Model* htn, searchNode* tnI){
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
}
