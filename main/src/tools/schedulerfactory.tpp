#ifndef SCHEDULERFACTORY_TPP
#define SCHEDULERFACTORY_TPP

#include "tools/synchronizedscheduler.h"
#include "tools/unsynchronizedscheduler.h"
#include "tools/msgscheduler.h"
#include <boost/heap/binomial_heap.hpp>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <boost/heap/skew_heap.hpp>
#include <boost/heap/d_ary_heap.hpp>
#include "tools/msgscheduler.h"
#include "tools/vectorscheduler.h"
#include "tools/listscheduler.h"

namespace n_tools {


template<typename X>
typename SchedulerFactory<X>::t_Scheduler SchedulerFactory<X>::makeScheduler(const Storage& stype, bool synchronized, bool map_backed )
{
        if(map_backed && !synchronized){
                return t_Scheduler(new MessageScheduler<boost::heap::fibonacci_heap<X>, X>);
        }
	switch (stype) {
	case Storage::FIBONACCI: {
		if(synchronized)
			return t_Scheduler(new SynchronizedScheduler<boost::heap::fibonacci_heap<X>, X>);
		return t_Scheduler(new UnSynchronizedScheduler<boost::heap::fibonacci_heap<X>, X>);
	}
	case Storage::PAIRING: {
		if(synchronized)
			return t_Scheduler(new SynchronizedScheduler<boost::heap::pairing_heap<X>, X>);
		return t_Scheduler(new UnSynchronizedScheduler<boost::heap::pairing_heap<X>, X>);
	}
	case Storage::SKEW: {
		if(synchronized)
			return t_Scheduler(new SynchronizedScheduler<boost::heap::skew_heap<X, boost::heap::mutable_<true>>, X>);
		return t_Scheduler(new UnSynchronizedScheduler<boost::heap::skew_heap<X, boost::heap::mutable_<true>>, X>);
	}
	case Storage::LIST:{
		if(synchronized)
			return t_Scheduler(new SyncedListscheduler<X>);
		return t_Scheduler(new Listscheduler<X>);
	}
	default:
		assert(false && "No such storage type");
		return nullptr;
	}
}
}
#endif
