/*
 * Allocator.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_ALLOCATOR_H_
#define SRC_CONTROL_ALLOCATOR_H_

namespace n_model {
class AtomicModel;
typedef std::shared_ptr<AtomicModel> t_atomicmodelptr;
}

namespace n_control {

class Allocator
{
public:
	Allocator();

	/*
	 * Decide on which core to place a model
	 */
	virtual size_t allocate(n_model::t_atomicmodelptr) = 0;

	virtual ~Allocator();
};

} /* namespace n_control */

#endif /* SRC_CONTROL_ALLOCATOR_H_ */
