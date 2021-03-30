#include <vector>
#include <iostream>
#include <queue>
#include <cassert>
#include "Invariants.h"
#include "Debug.h"

using namespace std;

namespace std {
template <> struct hash<std::pair<int, int>> {
    inline size_t operator()(const std::pair<int, int> &v) const {
        std::hash<int> int_hasher;
        return int_hasher(v.first) ^ int_hasher(v.second);
    }
};

}

// note: all invariants are contained twice!
unordered_set<int> * binary_invariants;

bool can_state_features_co_occur(Model * htn, int a, int b){
	if (a == -b-1) return false; // edge case!
	a = -a-1 + htn->numStateBits;
	b = -b-1;


	return !binary_invariants[a].count(b);
}

void insert_invariant(Model * htn, int a, int b){
	//cout << "\t\t\t\t\t\t\t\t\tInsert " << a << " " << b << endl;
	binary_invariants[a + htn->numStateBits].insert(b);
	binary_invariants[b + htn->numStateBits].insert(a);
}

int count_invariants(Model * htn){
	int number_of_invariants = 0;
	for (int i = 0; i < 2*htn->numStateBits; i++)
		number_of_invariants += binary_invariants[i].size();
	return number_of_invariants / 2; // count double
}

void extract_invariants_from_parsed_model(Model * htn){
	cout << endl << "Extracting invariants from parsed model" << endl;
	binary_invariants = new unordered_set<int>[2*htn->numStateBits];

	int unusableInvariant = 0;
	int unusableStrictMutexes = 0;
	int unusableSASGroups = 0;

	// go through everything we know and add invariants
	for (size_t invarN = 0; invarN < htn->numInvariants; invarN++){
		if (htn->invariantsSize[invarN] != 2) { unusableInvariant++; continue; }

		insert_invariant(htn,htn->invariants[invarN][0], htn->invariants[invarN][1]);
	}
		
	for (size_t mutexN = 0; mutexN < htn->numStrictMutexes; mutexN++){
		for (size_t m1 = 0; m1 < htn->strictMutexesSize[mutexN]; m1++)
			for (size_t m2 = m1 + 1; m2 < htn->strictMutexesSize[mutexN]; m2++)
				insert_invariant(htn,-htn->strictMutexes[mutexN][m1]-1, -htn->strictMutexes[mutexN][m2]-1);

		if (htn->strictMutexesSize[mutexN] != 2){ unusableStrictMutexes++; continue; }
		// if it is strict, one of the two values must be true
		insert_invariant(htn,htn->strictMutexes[mutexN][0], htn->strictMutexes[mutexN][1]);
	}



	for (size_t mutexN = 0; mutexN < htn->numMutexes; mutexN++){
		for (size_t m1 = 0; m1 < htn->mutexesSize[mutexN]; m1++)
			for (size_t m2 = m1 + 1; m2 < htn->mutexesSize[mutexN]; m2++)
				insert_invariant(htn,-htn->mutexes[mutexN][m1]-1, -htn->mutexes[mutexN][m2]-1);
	}

	// SAS+ groups imply mutual exclusion
	for (size_t var = 0; var < htn->numVars; var++){
		for (int i = htn->firstIndex[var]; i <= htn->lastIndex[var]; i++)
			for (int j = i+1; j <= htn->lastIndex[var]; j++)
				insert_invariant(htn,-i-1, -j-1);	
	
		if (htn->lastIndex[var] - htn->firstIndex[var] != 1) { unusableSASGroups++; continue; }
		insert_invariant(htn, htn->firstIndex[var], htn->lastIndex[var]);
	}



	// see how many invariants we got for statistics
	int number_of_invariants = count_invariants(htn);
	cout << "Extracted " << number_of_invariants << " invariants." << endl;

	
	cout << "Starting resolution." << endl;
	std::clock_t resolution_start = std::clock();
	
	// now do resolution over the invariants
	queue<pair<int,int>> invariant_to_process;
	for (int i = 0; i < 2*htn->numStateBits; i++){
		int a = i - htn->numStateBits;
		if (a < 0) a = -a-1;
		for (const int & j : binary_invariants[i]){
			int b = j;
			if (b < 0) b = -b-1;
			if (a < b){
				invariant_to_process.push(make_pair(i - htn->numStateBits,j));
			}
		}
	}

	int round = 0;
	int newInvars = 0;
	while (invariant_to_process.size()){
		while (invariant_to_process.size()){
			pair<int,int> inv = invariant_to_process.front();
			invariant_to_process.pop();
			int invA = -inv.first - 1;
			int invB = -inv.second - 1;

			// first variable, look for invariants containing the contrary fact
			for (const int & j : binary_invariants[invA + htn->numStateBits]){
				if (-j-1 == inv.second) continue; // trivial invariant
				// the new invariant is inv.second,j
				if (!binary_invariants[inv.second + htn->numStateBits].count(j)){
					insert_invariant(htn,inv.second,j);
					invariant_to_process.push(make_pair(inv.second,j));
					newInvars++;
				}
			}

			// second variable, look for invariants containing the contrary fact
			for (const int & j : binary_invariants[invB + htn->numStateBits]){
				if (-j-1 == inv.first) continue; // trivial invariant
				// the new invariant is inv.first,j
				if (!binary_invariants[inv.first + htn->numStateBits].count(j)){
					insert_invariant(htn,inv.first,j);
					invariant_to_process.push(make_pair(inv.first,j));
					newInvars++;
				}
			}
			round++;
			if (round % 500 == 0){
				cout << "Resolution: " << invariant_to_process.size() << " remaining. " << newInvars << " new invariants found." << endl;
			}
		}

		cout << "Starting inference from SAS groups." << endl;
		// the SAS group is essentially a big OR, if we can resolve every member except for one with the same clause we have a new clause
		for (int iAccess = 0; iAccess < 2*htn->numStateBits; iAccess++){
			// check for all mutex groups whether it is mutex with all but one of the elements
			for (size_t var = 0; var < htn->numVars; var++){
				if (htn->firstIndex[var] == htn->lastIndex[var]) continue; // no true SAS group
				int pred = iAccess - htn->numStateBits; if (pred < 0) pred = -pred-1;
				if (htn->firstIndex[var] <= pred && pred <= htn->lastIndex[var]) continue; // no inference with my own SAS group
		

				int invar = 0; // 0 all mutex, 1 more than two non mutex
				for (int i = htn->firstIndex[var]; i <= htn->lastIndex[var]; i++){
					int iNeg = -i-1;
					if (binary_invariants[iAccess].count(iNeg)) continue;
					if (invar == 0) invar = iNeg;
					else if (invar < 0) { invar = 1; break; }
				}

				if (invar == 1) continue; // cannot be mutex
				if (invar == 0){
					cout << "Fact " << htn->factStrs[pred] << " (" << pred << ") is statically " << (iAccess < htn->numStateBits ? "false" : "true") << "." << endl;
					cout << "This was shown using the SAS+ variable " << htn->varNames[var] << "." << endl;
				   	cout << "This should have been detected in preprocessing ... I'm giving up." << endl;
					//exit(0);
				}
				if (invar < 0 && binary_invariants[iAccess].count(-invar-1) == 0){
					// so only invar is the only one that is not mutex with iAccess, thus we have a new invariant
					insert_invariant(htn, -invar-1, iAccess - htn->numStateBits);
					invariant_to_process.push(make_pair(-invar-1, iAccess - htn->numStateBits));
				}
			}
		}
		cout << "Found " << invariant_to_process.size() << " new invariants." << endl;
	}

	number_of_invariants = count_invariants(htn);
	std::clock_t resolution_end = std::clock();
	double resolution_time = 1000.0 * (resolution_end-resolution_start) / CLOCKS_PER_SEC;
	
	cout << "After resolution we have " << number_of_invariants << " invariants, taking " << resolution_time << "ms." << endl;
}


void compute_Rintanen_initial_invariants(Model * htn,
		vector<pair<int,int>> & v0,
		bool * & toDelete,
		vector<vector<int>> & posInvarsPerPredicate,
		vector<vector<int>> & negInvarsPerPredicate
	){
	bool * s0Vector = new bool[htn->numStateBits];
	for (int i = 0; i < htn->numStateBits; i++) s0Vector[i] = false;
	for (int i = 0; i < htn->s0Size; i++) s0Vector[htn->s0List[i]] = true;


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
	
	// create delete list
	toDelete = new bool[v0.size()];
	for (size_t i = 0; i < v0.size(); i++)
	toDelete[i] = false;
}

void compute_Rintanen_reduce_invariants(Model * htn,
		vector<pair<int,int>> & v0,
		bool * & toDelete,
		vector<vector<int>> & posInvarsPerPredicate,
		vector<vector<int>> & negInvarsPerPredicate
	){
	// create new copies of everything
	vector<pair<int,int>> v1;
	vector<vector<int>> posInvarsPerPredicate_new;
	vector<vector<int>> negInvarsPerPredicate_new;
	for (int p1 = 0; p1 < htn->numStateBits; p1++){
		vector<int> _v;
		posInvarsPerPredicate_new.push_back(_v);
		negInvarsPerPredicate_new.push_back(_v);
	}

	// copy only the things we need to keep
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

	// swap into the old variables -> these are
	swap(v0,v1); // constant time
	swap(posInvarsPerPredicate, posInvarsPerPredicate_new);
	swap(negInvarsPerPredicate, negInvarsPerPredicate_new);
	
	
	// re-create delete list
	delete[] toDelete;
	toDelete = new bool[v0.size()];
	for (size_t i = 0; i < v0.size(); i++) toDelete[i] = false;
}

// return true if action is applicable  
bool compute_Rintanten_action_applicable(Model * htn,
		int tIndex,
		vector<pair<int,int>> & v0,
		bool * & toDelete,
		vector<vector<int>> & posInvarsPerPredicate,
		vector<vector<int>> & negInvarsPerPredicate,
		bool * & posInferredPreconditions,
		bool * & negInferredPreconditions
	){
	
	// infer additional preconditions and effects
	for (int p1 = 0; p1 < htn->numStateBits; p1++){
		posInferredPreconditions[p1] = false;
		negInferredPreconditions[p1] = false;
	}

	bool inapplicable = false;
	for (size_t preIndex = 0; !inapplicable && preIndex < htn->numPrecs[tIndex]; preIndex++){
		int pre = htn->precLists[tIndex][preIndex];
		posInferredPreconditions[pre] = true;

		// look only at the invariants that are possibly matching this precondition
		for (size_t invarListIndex = 0; invarListIndex < negInvarsPerPredicate[pre].size(); invarListIndex++){
			int invar = negInvarsPerPredicate[pre][invarListIndex];
			if (toDelete[invar]) continue;
			if (v0[invar].first < 0 && pre == -v0[invar].first-1){
				if (v0[invar].second < 0){
					negInferredPreconditions[-v0[invar].second - 1] = true;
					if (posInferredPreconditions[-v0[invar].second - 1]) {inapplicable = true; break;}
				} else {
					posInferredPreconditions[ v0[invar].second] = true;
					if (negInferredPreconditions[v0[invar].second]) {inapplicable = true; break;}
				}
			}
			if (v0[invar].second < 0 && pre == -v0[invar].second-1){
				if (v0[invar].first < 0){
					negInferredPreconditions[-v0[invar].first - 1] = true;
					if (posInferredPreconditions[-v0[invar].first - 1]) {inapplicable = true; break;}
				} else {
					posInferredPreconditions[ v0[invar].first] = true;
					if (negInferredPreconditions[v0[invar].first]) {inapplicable = true; break;}
				}
			}
		}
	}
	return !inapplicable;
}


// returns number of removed invariants
bool compute_Rintanten_action_effect(Model * htn,
		int tIndex,
		vector<pair<int,int>> & v0,
		bool * & toDelete,
		vector<vector<int>> & posInvarsPerPredicate,
		vector<vector<int>> & negInvarsPerPredicate,
		bool * & posInferredPreconditions,
		bool * & negInferredPreconditions
	){

	int removedInvariants = 0;

#define ensuresNegP(p) (negInferredPreconditions[p] && !htn->addVectors[tIndex][p]) || htn->delVectors[tIndex][p]
#define ensuresPosP(p) (posInferredPreconditions[p] && !htn->delVectors[tIndex][p]) || htn->addVectors[tIndex][p]
	for (int type = 0 ; type < 2; type++){
		int num   = type ? htn->numAdds[tIndex] : htn->numDels[tIndex];
		int* list = type ? htn->addLists[tIndex] : htn->delLists[tIndex];
		vector<vector<int>> & invarSource = type ? negInvarsPerPredicate : posInvarsPerPredicate; 
		
		for (size_t index = 0; index < num; index++){
			int c = list[index];
			// if the actions adds something, this may violate an invariant containing the predicate negatively
			for (size_t invarListIndex = 0; invarListIndex < invarSource[c].size(); invarListIndex++){
				int invar = invarSource[c][invarListIndex];
				if (toDelete[invar]) continue;
			
				// so this invariant is violated if it does not ensure the other literal
    	        bool ab = v0[invar].first  >= 0;
    	        bool bb = v0[invar].second >= 0;
				int ap = v0[invar].first;   if (!ab) ap = -ap - 1;
				int bp = v0[invar].second;  if (!bb) bp = -bp - 1;
			
				if (ap == c){
					// check if (bp,bb) is ensured by the action
					if (bb){
						if (ensuresPosP(bp)) continue;
					} else {
						if (ensuresNegP(bp)) continue;
					}
				} else {
					if (ab){
						if (ensuresPosP(ap)) continue;
					} else {
						if (ensuresNegP(ap)) continue;
					}
				}
				
				toDelete[invar] = true;
				removedInvariants++;
			}
		}
	}
	return removedInvariants;
}


void compute_Rintanen_Invariants(Model * htn){
	std::clock_t invariant_start = std::clock();
	cout << endl << "Computing invariants [Rintanen]" << endl;
		
	vector<pair<int,int>> v0;
	bool * toDelete;
	vector<vector<int>> posInvarsPerPredicate;
	vector<vector<int>> negInvarsPerPredicate;

	compute_Rintanen_initial_invariants(htn,v0,toDelete,posInvarsPerPredicate,negInvarsPerPredicate);
	cout << "Initial candidates build (" << v0.size() << ")" << endl;
	
	
	bool * posInferredPreconditions = new bool[htn->numStateBits];
	bool * negInferredPreconditions = new bool[htn->numStateBits];
	
	
	int nc = 0;
	int round = 1;
	int lastChangeAtAction = -1;
	do {
		cout << "Round " << round;
		nc = 0;

		// reduce data structures
		if (round != 1) compute_Rintanen_reduce_invariants(htn, v0, toDelete, posInvarsPerPredicate, negInvarsPerPredicate);
		round++;


		cout << ": " << v0.size() << " invariants remaining" << endl;

		for (size_t tIndex = 0; tIndex < htn->numActions; tIndex++){
			// we can break if we already have done a full round without any change
			if (!nc && tIndex > lastChangeAtAction) break;
		
			if (!compute_Rintanten_action_applicable(htn,tIndex,v0,toDelete, posInvarsPerPredicate, negInvarsPerPredicate, posInferredPreconditions, negInferredPreconditions))
				continue;

			int removedInvariants = 
				compute_Rintanten_action_effect(htn,tIndex,v0,toDelete, posInvarsPerPredicate, negInvarsPerPredicate, posInferredPreconditions, negInferredPreconditions);
				
			if (removedInvariants){	
				nc += removedInvariants;
				lastChangeAtAction = tIndex;
			}

			if (tIndex % 5000 == 4999)
				cout << "  " << v0.size() - nc << " / " << v0.size() << " remaining after " << tIndex+1 << " of " << htn->numActions << endl;
		}
	} while (nc);

	std::clock_t invariant_end = std::clock();
	double invariant_time = 1000.0 * (invariant_end-invariant_start) / CLOCKS_PER_SEC;
	
	cout << "Found " << v0.size() << " invariants in " << invariant_time << "ms" << endl;


	// debugging, see whether H2 mutexes are better or not?
#ifdef FALSE
	int t = 0, k = 0;
	for (auto [a,b] : v0){
		bool known = false;
		// check whether this mutex is implied
		// look through the invariants

		t++;
		if (binary_invariants[a + htn->numStateBits].count(b)) continue;	
		k++;
		cout << "New invariant: ";

		set<int> xx; xx.insert(a); xx.insert(b);
		for (int y : xx){
			cout << " " << y << " (";
			int p = y;
			if (p < 0) cout << "not ", p = -p - 1;
			cout << htn->factStrs[p];
			cout << ")";
		}

		cout << endl;
		
		//cout << k << "/" << t << endl;
	}

	cout << "Total invariants " << t << " new ones " << k << endl;


	unordered_set<pair<int,int>> v0set;
	for (const auto & x : v0) v0set.insert(x);

	for (int i = 0; i < 2*htn->numStateBits; i++){
		int a = i - htn->numStateBits;
		for (const int & j : binary_invariants[i]){
			int b = j;
			if (v0set.count(make_pair(a,b)) || v0set.count(make_pair(b,a))) continue;

			cout << "Unknown invariant: ";
	
			set<int> xx; xx.insert(a); xx.insert(b);
			for (int y : xx){
				cout << " " << y << " (";
				int p = y;
				if (p < 0) cout << "not ", p = -p - 1;
				cout << htn->factStrs[p];
				cout << ")";
			}
	
			cout << endl;
		}
	}
#endif


	// add invariants to list
	for (auto [a,b] : v0)
		insert_invariant(htn,a,b);
}
