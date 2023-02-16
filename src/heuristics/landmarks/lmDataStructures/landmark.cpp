/*
 * landmark.cpp
 *
 *  Created on: 12.02.2020
 *      Author: dh
 */

#include "landmark.h"
#include <iostream>

using namespace std;

namespace progression {

    landmark::landmark() {
        // TODO Auto-generated constructor stub

    }

    landmark::landmark(lmConType connection, lmType type, int size) {
        this->connection = connection;
        this->type = type;
        this->size = size;
        this->lm = new int[size];
    }

    landmark::~landmark() {
        delete[] this->lm;
    }

    void landmark::printLM() {
        cout << "LM ";
        if (this->type == fact) {
            cout << "fact";
        } else if (this->type == task) {
            cout << "task";
        } else if (this->type == METHOD) {
            cout << "meth";
        }
        cout << " ";

        if (this->connection == atom) {
            cout << "atom";
        } else if (this->connection == conjunctive) {
            cout << "conj";
        } else if (this->connection == disjunctive) {
            cout << "disj";
        }
        for (int j = 0; j < this->size; j++) {
            cout << " " << this->lm[j];
        }
        cout << endl;
    }

    int landmark::coutLM(landmark **lm, lmType type, int size) {
        int res = 0;
        for (int i = 0; i < size; i++) {
            if (lm[i]->type == type)
                res++;
        }
        return res;
    }
} /* namespace progression */
