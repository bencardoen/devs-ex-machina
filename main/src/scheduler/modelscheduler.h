/*
 * modelscheduler.h
 *
 *  Created on: Oct 21, 2015
 *      Author: Devs Ex Machina
 */

#ifndef SRC_SCHEDULER_MODELSCHEDULER_H_
#define SRC_SCHEDULER_MODELSCHEDULER_H_

#include "scheduler/modelheapscheduler.h"
#include "scheduler/genericmodelscheduler.h"
#include "model/atomicmodel.h"

namespace n_tools {

template<template<typename...T> class Container, template<typename...T> class Heap>
struct ModelScheduler
{
	typedef GenericModelScheduler<Container, Heap> t_type;
};

template<template<typename...T> class Heap>
struct ModelScheduler<ModelHeapScheduler, Heap>
{
	typedef ModelHeapScheduler<n_model::t_raw_atomic> t_type;
};

typedef ModelScheduler<ModelHeapScheduler, std::vector>::t_type t_defaultModelScheduler;
typedef ModelScheduler<n_tools::VectorScheduler, boost::heap::pairing_heap>::t_type t_Vector_PairingHeap_scheduler;

} /* namespace n_tools */




#endif /* SRC_SCHEDULER_MODELSCHEDULER_H_ */
