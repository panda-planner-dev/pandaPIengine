/*
 * LmFdConnector.cpp
 *
 *  Created on: 09.02.2020
 *      Author: dh
 */

#include "LmFdConnector.h"

namespace progression {

LmFdConnector::LmFdConnector() {
	// TODO Auto-generated constructor stub

}

LmFdConnector::~LmFdConnector() {
	// TODO Auto-generated destructor stub
}

int LmFdConnector::getNumLMs(){
	return this->numLMs;
}

landmark** LmFdConnector::getLMs(){
	// todo: should be cloned and destroyed
	return this->landmarks;
}

void LmFdConnector::createLMs(Model* htn) {
	cout << "Connector for FD landmark generation" << endl;
	RCModelFactory* factory = new RCModelFactory(htn);
	Model* rc = factory->getRCmodelSTRIPS();
	cout << "- Writing PDDL model to generate FD landmarks...";
	rc->writeToPDDL("/home/dh/Schreibtisch/temp/d.pddl", "/home/dh/Schreibtisch/temp/p.pddl");
	cout << "(done)." << endl;

	cout << "- Calling FD landmark generator...";
	system("\"/home/dh/Schreibtisch/temp/callFD.sh\"");
	cout << "(done)." << endl;

	cout << "- Reading FD landmarks...";
	this->readFDLMs("/home/dh/Schreibtisch/temp/fd.out", factory);
	//this->readFDLMs("/home/dh/Schreibtisch/temp/lmc-out.txt", factory);
	cout << "(done)." << endl;
	cout << "- Found " << this->numLMs << " landmarks (well, FD did...)" << endl;
	cout << "- " << this->numConjunctive << " are conjunctive" << endl;

	/*
	for(int j = 0; j < this->numLMs; j++) {
		int* l = this->landmarks[j];
		cout << "-- " << l[0] << " " << l[1]  << " " << l[2] << " ";
		for(int i = 0;  i < l[2]; i++) {
			if(l[1] == factory->fTask) {
				cout << factory->htn->taskNames[l[i + 3]] << " ";
			} else {
				cout << factory->htn->factStrs[l[i + 3]] << " ";
			}
		}
		cout << endl;
	}*/
}

void LmFdConnector::readFDLMs(string f, RCModelFactory* factory) {
	list<landmark*> lms;
	const char *cstr = f.c_str();
	ifstream domainFile(cstr);
	string line;
	getline(domainFile, line);
	numConjunctive = 0;

	while(line.compare("digraph G {") != 0) {
		getline(domainFile, line);
	}
	while(line.compare("}") != 0) {
		getline(domainFile, line);
		if(line.rfind("LM ", 0) == 0) { // that's C++ for "startswith" ;-)
			string s = su.getStrX(line, 2);
			if((s.compare("conj") == 0) || (s.compare("disj") == 0)){
				numConjunctive++;
				set<int> lm;
				string sLM = su.getStrX(line, 4);
			    pair<int, int> lmAtom = factory->rcStateFeature2HtnIndex(sLM);
				lm.insert(lmAtom.second);
				int type = lmAtom.first;

				int j = 6;
				while(su.getStrX(line, j).compare("Atom") == 0) {
					sLM = su.getStrX(line, j + 1);
					lmAtom = factory->rcStateFeature2HtnIndex(sLM);
					lm.insert(lmAtom.second);
					assert(type == lmAtom.first);
					j+=2;
				}

				landmark* r = new landmark();
				if(s.compare("conj") == 0)
					r->connection = conjunctive;
				if(s.compare("disj") == 0)
					r->connection = disjunctive;

				if(type == factory->fFact)
					r->type = fact;
				else
					r->type = task;
				r->size = lm.size();
				r->lm = new int[r->size];
				int ri = 0;
				for(int k : lm) {
					r->lm[ri++] = k;
				}
				lms.push_back(r);
			} else if(s.compare("Atom") == 0) {
				string sLM = su.getStrX(line, 3);
			    pair<int, int> lmAtom = factory->rcStateFeature2HtnIndex(sLM);

				landmark* r = new landmark();
				r->connection = atom;
				if(lmAtom.first == factory->fFact)
					r->type = fact;
				else
					r->type = task;
				r->size = 1;
				r->lm = new int[1];
				r->lm[0] = lmAtom.second;
				lms.push_back(r);
			}
		}
	}
	numLMs = lms.size();
	landmarks = new landmark*[numLMs];
	int i = 0;
	for(landmark* l: lms) {
		landmarks[i++] = l;
	}
}
} /* namespace progression */
