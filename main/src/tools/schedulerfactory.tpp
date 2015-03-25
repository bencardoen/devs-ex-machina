#ifndef SCHEDULERFACTORY_TPP
#define SCHEDULERFACTORY_TPP

#include "synchronizedscheduler.h"
#include "unsynchronizedscheduler.h"
#include <boost/heap/binomial_heap.hpp>
#include <boost/heap/priority_queue.hpp>
#include <boost/heap/fibonacci_heap.hpp>

namespace n_tools {
//enum class SchedulerStorageType{PQUEUE, FIBONACCI, BINOMIAL};

template<typename X>
typename SchedulerFactory<X>::t_Scheduler SchedulerFactory<X>::makeScheduler(const Storage& stype, bool synchronized )
{
	switch (stype) {
	case Storage::FIBONACCI: {
        if(synchronized)
            return t_Scheduler(new UnSynchronizedScheduler<boost::heap::fibonacci_heap<X>, X>);
	return t_Scheduler(new SynchronizedScheduler<boost::heap::fibonacci_heap<X>, X>);
	}
	case Storage::BINOMIAL: {
        if(synchronized)
            return t_Scheduler(new UnSynchronizedScheduler<boost::heap::binomial_heap<X>, X>);
	return t_Scheduler(new SynchronizedScheduler<boost::heap::binomial_heap<X>, X>);
	}
	default:
		assert(false && "No such storage type");
		return nullptr;
	}
}
}
#endif
