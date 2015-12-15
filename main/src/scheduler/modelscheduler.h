/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve
 */

#ifndef SRC_SCHEDULER_MODELSCHEDULER_H_
#define SRC_SCHEDULER_MODELSCHEDULER_H_

#include "scheduler/modelheapscheduler.h"
#include "scheduler/genericmodelscheduler.h"
#include "model/atomicmodel.h"

namespace n_scheduler {

/**
 * @brief Helper struct for easier creation of schedulers.
 * @tparam Container The top level container of the model scheduler.
 * @tparam Heap If applicable, the underlying heap structure.
 */
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
typedef ModelScheduler<n_scheduler::VectorScheduler, boost::heap::pairing_heap>::t_type t_Vector_PairingHeap_scheduler;

} /* namespace n_scheduler */




#endif /* SRC_SCHEDULER_MODELSCHEDULER_H_ */
