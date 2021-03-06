/*
 * IntPairHeap.h
 *
 *  Created on: 29.09.2017
 *      Author: Daniel Höller
 */

#ifndef INTPAIRHEAP_H_
#define INTPAIRHEAP_H_

#include "flags.h"

namespace progression {

class IntPairHeap {
private:
    tHVal* keys;
    tHVal* vals;

    int nextIndex;
    int arraySize;
public:
	IntPairHeap(int size);
	virtual ~IntPairHeap();

	bool isEmpty();
	int size();
	void clear();

	void add(tHVal key, tHVal val);
	void pop();
    tHVal topVal();
    tHVal topKey();
};

} /* namespace progression */

#endif /* INTPAIRHEAP_H_ */
