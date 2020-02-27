/*
 * bIntSet.h
 *
 *  Created on: 05.10.2017
 *      Author: Daniel Höller
 */

#ifndef INTDATASTRUCTURES_BINTSET_H_
#define INTDATASTRUCTURES_BINTSET_H_

namespace progression {

class bIntSet {
private:
	bool* boolContainer;
	int* intContainer;
	int iterI = -1;
	int containerSize;
	int currentSize = -1;

public:
	bIntSet();
	virtual ~bIntSet();

	void sortSet();
	void sort(int* ints, int minIndex, int maxIndex);
	int partition(int* ints, int minIndex, int maxIndex);
	int find(int* ints, int minIndex, int maxIndex, int element);

	void init(int size);
	int getSize();
	void insert(int i);
	void append(int i);
	void erase(int i);
	bool isEmpty();
	void clear();
	int getFirst();
	int getNext();
	bool get(int i);
};

} /* namespace progression */

#endif /* INTDATASTRUCTURES_BINTSET_H_ */
