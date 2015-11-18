#ifndef SCHEDULERFACTORY_TPP
#define SCHEDULERFACTORY_TPP

#include <boost/heap/binomial_heap.hpp>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <boost/heap/skew_heap.hpp>
#include <boost/heap/d_ary_heap.hpp>
#include <scheduler/synchronizedscheduler.h>
#include <scheduler/unsynchronizedscheduler.h>
#include "scheduler/msgscheduler.h"
#include "scheduler/simplemsgscheduler.h"

#include "scheduler/msgscheduler.h"
#include "scheduler/listscheduler.h"

namespace n_scheduler {


template<typename X>
typename SchedulerFactory<X>::t_Scheduler SchedulerFactory<X>::makeScheduler(Storage stype, bool synchronized, KeyStorage ktype )
{
        // TODO specialize, else types for which operator size_t is not defined fail to compile despite not being used.
        
        if(ktype==KeyStorage::MAP && !synchronized)
                return t_Scheduler(new MessageScheduler<boost::heap::fibonacci_heap<X>, X>);        
        else if(ktype==KeyStorage::NONE && !synchronized)
                return t_Scheduler(new SimpleMessageScheduler<boost::heap::fibonacci_heap<X>, X>);
        if(ktype!=KeyStorage::HASHMAP)
                throw std::logic_error("Unsupported keytype.");
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
