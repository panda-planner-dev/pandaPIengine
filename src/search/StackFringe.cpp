/*
 * StackFringe.cpp
 *
 *  Created on: 18.12.2018
 *      Author: Daniel Höller
 */

#include "StackFringe.h"

namespace progression {

StackFringe::StackFringe() {
	f = new UnsortedFringe;
}

StackFringe::~StackFringe() {
	delete f;
}

void StackFringe::push(searchNode* n) {
	f->addLast(n);
}

bool StackFringe::empty() {
	return f->empty();
}

searchNode* StackFringe::top() {
	return f->removeLast();
}

void StackFringe::pop() {

}

int StackFringe::size() {
	return f->size();
}

} /* namespace progression */
