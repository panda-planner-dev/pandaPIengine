#include <vector>
#include <iostream>
#include "Invariants.h"

using namespace std;

void compute_Rintanen_Invariants(Model * htn){
	std::clock_t invariant_start = std::clock();
	cout << "Computing invariants [Rintanen]" << endl;

	bool * s0Vector = new bool[htn->numStateBits];
	for (int i = 0; i < htn->numStateBits; i++) s0Vector[i] = false;
	for (int i = 0; i < htn->s0Size; i++) s0Vector[htn->s0List[i]] = true;


	vector<pair<int,int>> v0;
	vector<vector<int>> posInvarsPerPredicate;
	vector<vector<int>> negInvarsPerPredicate;
	for (int p1 = 0; p1 < htn->numStateBits; p1++){
		vector<int> _v;
		posInvarsPerPredicate.push_back(_v);
		negInvarsPerPredicate.push_back(_v);
	}
	
	
	for (int p1 = 0; p1 < htn->numStateBits; p1++){
		for (int p2 = p1+1; p2 < htn->numStateBits; p2++){
			for (int b1 = 0; b1 < 2; b1++){
				for (int b2 = 0; b2 < 2; b2++){
					if (b1 == s0Vector[p1] && b2 == s0Vector[p2]) continue;
					int l1 = p1; if (b1) l1 = -p1-1;
					int l2 = p2; if (b2) l2 = -p2-1;
					int invarNum = v0.size();

					if (b1)
						negInvarsPerPredicate[p1].push_back(invarNum);
					else
						posInvarsPerPredicate[p1].push_back(invarNum);

					if (b2)
						negInvarsPerPredicate[p2].push_back(invarNum);
					else
						posInvarsPerPredicate[p2].push_back(invarNum);
					
					v0.push_back(make_pair(l1,l2));
				}
			}
		}
	}
	cout << "Initial candidates build (" << v0.size() << ")" << endl;


	bool * posInferredPreconditions = new bool[htn->numStateBits];
	bool * negInferredPreconditions = new bool[htn->numStateBits];
	bool * ensuresPosP = new bool[htn->numStateBits];
	bool * ensuresNegP = new bool[htn->numStateBits];

	bool * toDelete = new bool[v0.size()];
	for (size_t i = 0; i < v0.size(); i++)
	toDelete[i] = false;
	
	
	int nc = 0;
	int round = 1;
	do {
		cout << "Round " << round;
		nc = 0;

		// reduce data structures
		if (round != 1){
			vector<pair<int,int>> v1;
			vector<vector<int>> posInvarsPerPredicate_new;
			vector<vector<int>> negInvarsPerPredicate_new;
			for (int p1 = 0; p1 < htn->numStateBits; p1++){
				vector<int> _v;
				posInvarsPerPredicate_new.push_back(_v);
				negInvarsPerPredicate_new.push_back(_v);
			}

			for (size_t i = 0; i < v0.size(); i++)
				if (!toDelete[i]){
					int num = v1.size();
					v1.push_back(v0[i]);
					int a = v0[i].first;  if (a < 0) a = -a-1;
					int b = v0[i].second; if (b < 0) b = -b-1;
					
					if (v0[i].first < 0)
						negInvarsPerPredicate_new[a].push_back(num);
					else
						posInvarsPerPredicate_new[a].push_back(num);
					
					if (v0[i].second < 0)
						negInvarsPerPredicate_new[b].push_back(num);
					else
						posInvarsPerPredicate_new[b].push_back(num);
				}

			swap(v0,v1); // constant time
			swap(posInvarsPerPredicate, posInvarsPerPredicate_new);
			swap(negInvarsPerPredicate, negInvarsPerPredicate_new);

			delete[] toDelete;
		}
		round++;

		// create delete list
		toDelete = new bool[v0.size()];
		for (size_t i = 0; i < v0.size(); i++)
		toDelete[i] = false;

		cout << ": " << v0.size() << " invariants remaining" << endl;

		for (size_t tIndex = 0; tIndex < htn->numActions; tIndex++){
			// infer additional preconditions and effects
			for (int p1 = 0; p1 < htn->numStateBits; p1++){
				posInferredPreconditions[p1] = false;
				negInferredPreconditions[p1] = false;
			}

			for (size_t preIndex = 0; preIndex < htn->numPrecs[tIndex]; preIndex++){
				int pre = htn->precLists[tIndex][preIndex];
				posInferredPreconditions[pre] = true;

				// look only at the invariants that are possibly matching this precondition
				// TODO look only at the ones containing it negatively	
				for (size_t invarListIndex = 0; invarListIndex < negInvarsPerPredicate[pre].size(); invarListIndex++){
					int invar = negInvarsPerPredicate[pre][invarListIndex];
					if (toDelete[invar]) continue;
					if (v0[invar].first < 0 && pre == -v0[invar].first-1){
						if (v0[invar].second < 0)
							negInferredPreconditions[-v0[invar].second - 1] = true;
						else
							posInferredPreconditions[ v0[invar].second] = true;
					}
					if (v0[invar].second < 0 && pre == -v0[invar].second-1){
						if (v0[invar].first < 0)
							negInferredPreconditions[-v0[invar].first - 1] = true;
						else
							posInferredPreconditions[ v0[invar].first] = true;
					}
				}
			}


			for (int p1 = 0; p1 < htn->numStateBits; p1++){
				if ((posInferredPreconditions[p1] && !htn->delVectors[tIndex][p1]) || htn->addVectors[tIndex][p1])
					ensuresPosP[p1] = true;
				else
					ensuresPosP[p1] = false;
			}
			
			for (int p1 = 0; p1 < htn->numStateBits; p1++){
				if ((negInferredPreconditions[p1] && !htn->addVectors[tIndex][p1]) || htn->delVectors[tIndex][p1])
					ensuresNegP[p1] = true;
				else
					ensuresNegP[p1] = false;
			}


			for (size_t addIndex = 0; addIndex < htn->numAdds[tIndex]; addIndex++){
				int add = htn->addLists[tIndex][addIndex];
				// if the actions adds something, this may violate an invariant containing the predicate negatively
				for (size_t invarListIndex = 0; invarListIndex < negInvarsPerPredicate[add].size(); invarListIndex++){
					int invar = negInvarsPerPredicate[add][invarListIndex];
					if (toDelete[invar]) continue;
				
					// so this invariant is violated if it does not ensure the other literal
    	            bool ab = v0[invar].first  >= 0;
    	            bool bb = v0[invar].second >= 0;
					int ap = v0[invar].first;   if (!ab) ap = -ap - 1;
					int bp = v0[invar].second;  if (!bb) bp = -bp - 1;
				
					if (ap == add){
						// check if (bp,bb) is ensured by the action
						if (bb){
							if (ensuresPosP[bp]) continue;
						} else {
							if (ensuresNegP[bp]) continue;
						}
					} else {
						if (ab){
							if (ensuresPosP[ap]) continue;
						} else {
							if (ensuresNegP[ap]) continue;
						}
					}
					
					toDelete[invar] = true;
					nc += 1;
				}
			}


			for (size_t delIndex = 0; delIndex < htn->numDels[tIndex]; delIndex++){
				int del = htn->delLists[tIndex][delIndex];
				// if the actions adds something, this may violate an invariant containing the predicate negatively
				for (size_t invarListIndex = 0; invarListIndex < posInvarsPerPredicate[del].size(); invarListIndex++){
					int invar = posInvarsPerPredicate[del][invarListIndex];
					if (toDelete[invar]) continue;
				
					// so this invariant is violated if it does not ensure the other literal
    	            bool ab = v0[invar].first  >= 0;
    	            bool bb = v0[invar].second >= 0;
					int ap = v0[invar].first;   if (!ab) ap = -ap - 1;
					int bp = v0[invar].second;  if (!bb) bp = -bp - 1;
				
					if (ap == del){
						// check if (bp,bb) is ensured by the action
						if (bb){
							if (ensuresPosP[bp]) continue;
						} else {
							if (ensuresNegP[bp]) continue;
						}
					} else {
						if (ab){
							if (ensuresPosP[ap]) continue;
						} else {
							if (ensuresNegP[ap]) continue;
						}
					}
					
					toDelete[invar] = true;
					nc += 1;
				}
			}

			if (tIndex % 500 == 499)
				cout << "  " << v0.size() - nc << " / " << v0.size() << " remaining." << endl;
		}
	} while (nc);

	std::clock_t invariant_end = std::clock();
	double invariant_time = 1000.0 * (invariant_end-invariant_start) / CLOCKS_PER_SEC;
	
	cout << "Found " << v0.size() << " invariants in " << invariant_time << "ms" << endl;
}
