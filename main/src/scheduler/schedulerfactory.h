/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#include "scheduler/scheduler.h"
#include "scheduler/vectorscheduler.h"

#ifndef SCHEDULERFACTORY_H
#define SCHEDULERFACTORY_H

namespace n_scheduler {

/**
 * @brief The Storage enum Defines a user preference for the storage type the
 * scheduler will use. This makes it somewhat easier to use, and does not
 * expose the template storage parameter.
 */
enum class Storage
{
	/** Node based, with amortized complexity for push_back(), can be faster than binomial.*/
	FIBONACCI,
	/** Complexity is not yet proven, but 'tends' to be faster. Node-based. */
	PAIRING,
	/** Self adjusting node based heap. Fast merge (not exposed). */
	SKEW,
	/**
	 * List based scheduler.
	 */
	LIST,
};

/**
 * For scheduler operations where contains()/erase() is required in O(1) or O(logn), 
 * define a storage backend here. 
 */
enum class KeyStorage
{
        /**
         * Don't care, so duplicate items scheduled is possible and erase/contains is very slow.
         */
        NONE,
        /**
         * Uses std::hash<T>operator(const T&)const to store ptrs to heap items for fast retrieval.
         * Slow push/pop, fast retrieval.
         */
        HASHMAP,
        /**
         * Uses std::less<T>operator(const T& left, const T& right)const for comparison
         * In heavy push/pop usage can be faster (but lookup becomes slower)
         */
        MAP,
        /**
         * Expects a well defined implementation of 
         *      operator size_t()const for type T, that converts to a unique id.
         *      Pointers to elements are stored using that index.
         * Ids should form a continuous series to conserve memory.
         */
        VECTOR
};


/**
 * Provides easy creation to Scheduler instances.
 * @example
 * auto scheduler = SchedulerFactory<int>::makeScheduler(Storage::FIBONACCI);
 * scheduler is of type std::shared_ptr<Scheduler<int>>;
 */
template<typename X>
class SchedulerFactory
{
private:
	SchedulerFactory() = delete;
public:
	typedef std::shared_ptr<Scheduler<X>> t_Scheduler;

	static std::shared_ptr<Scheduler<X>>
	makeScheduler(Storage, bool synchronized = false, KeyStorage=KeyStorage::HASHMAP);
};

}
#include <scheduler/schedulerfactory.tpp>

#endif // SCHEDULERFACTORY_H
