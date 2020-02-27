/*
 * FlexIntStack.h
 *
 *  Created on: 13.12.2018
 *      Author: dh
 */

#ifndef INTDATASTRUCTURES_FLEXINTSTACK_H_
#define INTDATASTRUCTURES_FLEXINTSTACK_H_

namespace progression {

class FlexIntStack {
private:
	int* intContainer;
	int iterI = -1;
	int containerSize;
	int currentSize = -1;
public:
	FlexIntStack();
	virtual ~FlexIntStack();

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

#endif /* INTDATASTRUCTURES_FLEXINTSTACK_H_ */
