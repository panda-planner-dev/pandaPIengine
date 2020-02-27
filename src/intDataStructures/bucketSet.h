/*
 * bucketSet.h
 *
 *  Created on: 30.09.2017
 *      Author: Daniel Höller
 */

#ifndef BUCKETSET_H_
#define BUCKETSET_H_

namespace progression {

class bucketSet {
private:
	bool* container;
	int numSet =0;
	int containersize;
	int currentSize;

	int iIter;
	int numIterated;
public:
	bucketSet();
	virtual ~bucketSet();

	void init(int size);
	int getSize();
	void insert(int i);
	void erase(int i);
	bool isEmpty();
	void clear();
	int getFirst();
	int getNext();
	int removeFirst();
	int removeNext();
	bool get(int i);
};

} /* namespace progression */

#endif /* BUCKETSET_H_ */
