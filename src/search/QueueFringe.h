/*
 * QueueFringe.h
 *
 *  Created on: 18.12.2018
 *      Author: dh
 */

#ifndef QUEUEFRINGE_H_
#define QUEUEFRINGE_H_

#include "../ProgressionNetwork.h"
#include "UnsortedFringe.h"

namespace progression {

class QueueFringe {
public:
	QueueFringe();
	virtual ~QueueFringe();
	void push(searchNode* n);
	bool empty();
	searchNode* top();
	void pop();
	int size();
private:
	UnsortedFringe* f;
};

} /* namespace progression */

#endif /* QUEUEFRINGE_H_ */
