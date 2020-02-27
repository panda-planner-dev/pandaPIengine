/*
 * IntStack.h
 *
 *  Created on: 30.09.2017
 *      Author: dh
 */

#ifndef INTDATASTRUCTURES_INTSTACK_H_
#define INTDATASTRUCTURES_INTSTACK_H_

namespace progression {

class IntStack {
private:
	int* intContainer;
	int iterI = -1;
	int containerSize;
	int currentSize = -1;
public:
	IntStack();
	virtual ~IntStack();

	void init(int size);
	int getSize();
	void push(int i);
	int pop();
	bool isEmpty();
	void clear();
	int getFirst();
	int getNext();
};

} /* namespace progression */

#endif /* INTDATASTRUCTURES_INTSTACK_H_ */
