/*
 * Allocator.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_ALLOCATOR_H_
#define SRC_CONTROL_ALLOCATOR_H_

#include <memory>
#include "atomicmodel.h"

/*
namespace n_model {
class AtomicModel;
typedef std::shared_ptr<AtomicModel> t_atomicmodelptr;
}*/

namespace n_control {

/**
 * @brief Decides on which Core to place each Model
 */
class Allocator
{
public:
	Allocator();

	/**
	 * @brief Returns on which Core to place a Model
	 * @param model the AtomicModel to be allocated
	 */
	virtual size_t allocate(const n_model::t_atomicmodelptr&) = 0;

	virtual ~Allocator();
};

} /* namespace n_control */

#endif /* SRC_CONTROL_ALLOCATOR_H_ */
