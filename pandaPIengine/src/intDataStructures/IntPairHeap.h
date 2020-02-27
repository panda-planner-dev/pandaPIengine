/*
 * IntPairHeap.h
 *
 *  Created on: 29.09.2017
 *      Author: Daniel Höller
 */

#ifndef INTPAIRHEAP_H_
#define INTPAIRHEAP_H_

namespace progression {

class IntPairHeap {
private:
    int* keys;
    int* vals;

    int nextIndex;
    int arraySize;
public:
	IntPairHeap(int size);
	virtual ~IntPairHeap();

	bool isEmpty();
	int size();
	void clear();

	void add(int key, int val);
	void pop();
	int topVal();
	int topKey();
};

} /* namespace progression */

#endif /* INTPAIRHEAP_H_ */
