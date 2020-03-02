/*
 * noDelSet.h
 *
 *  Created on: 30.09.2017
 *      Author: dh
 */

#ifndef NODELINTSET_H_
#define NODELINTSET_H_

#include "IntUtil.h"

namespace progression {

class noDelIntSet {
private:
	bool* boolContainer = nullptr;
	int* intContainer = nullptr;
	int iterI = -1;
	int containerSize = -1;
	int currentSize = -1;

	IntUtil iu;

public:
	noDelIntSet();
	virtual ~noDelIntSet();

	void init(int size);
	int getSize();
	void insert(int i);
	bool isEmpty();
	void clear();
	int getFirst();
	int getNext();
	bool get(int i);
	noDelIntSet* clone();
	void sort();

	noDelIntSet* setUnion(noDelIntSet* second);
	noDelIntSet* setIntersection(noDelIntSet* second);
	noDelIntSet* setMinus(noDelIntSet* second);

	void setUnion(noDelIntSet* result, noDelIntSet* second);
	void setIntersection(noDelIntSet* result, noDelIntSet* second);
	void setMinus(noDelIntSet* result, noDelIntSet* second);
};

} /* namespace progression */

#endif /* NODELINTSET_H_ */
