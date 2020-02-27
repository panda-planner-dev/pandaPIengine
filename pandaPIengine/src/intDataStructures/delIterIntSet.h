/*
 * delInterIntSet.h
 *
 *  Created on: 01.02.2019
 *      Author: dh
 */

#ifndef INTDATASTRUCTURES_DELITERINTSET_H_
#define INTDATASTRUCTURES_DELITERINTSET_H_

namespace progression {

struct llInt {
	bool isFirst = false;
	bool isLast = false;
	int value;
	llInt* prev;
	llInt* next;
};

/*
 * Enables the following operations:
 *
 * - constructor in O(m) (m being maximal value insertable into the set)
 * - insertion in O(1)
 * - contains test in O(1)
 * - iterate to next element in O(1)
 * - delete current iterator element in O(1)
 * - clearing in O(n) (n being the current number of elements)
 */
class delIterIntSet {
private:
	bool* boolContainer = nullptr;
	int iterI = -1;
	int containerSize = -1;
	int currentSize = -1;
	llInt* first = nullptr;
	llInt* last = nullptr;
	llInt* currentIterElem = nullptr;
public:
	delIterIntSet();
	virtual ~delIterIntSet();

	void init(int size);
	int getSize();
	void insert(int i);
	bool isEmpty();
	void clear();
	int getFirst();
	int getNext();
	int delCurrentGetNext();
	bool get(int i);
};

} /* namespace progression */

#endif /* INTDATASTRUCTURES_DELITERINTSET_H_ */
