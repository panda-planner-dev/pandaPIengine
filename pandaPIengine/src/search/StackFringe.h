/*
 * StackFringe.h
 *
 *  Created on: 18.12.2018
 *      Author: dh
 */

#ifndef STACKFRINGE_H_
#define STACKFRINGE_H_

#include "../ProgressionNetwork.h"
#include "UnsortedFringe.h"
//#include <stack>

namespace progression {

class StackFringe {
public:
	StackFringe();
	virtual ~StackFringe();
	void push(searchNode* n);
	bool empty();
	searchNode* top();
	void pop();
	int size();
private:
	  //std::stack<searchNode*> f;
	  UnsortedFringe* f;
};

} /* namespace progression */

#endif /* STACKFRINGE_H_ */
