/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <memory>
#include "tools/globallog.h"
#include <vector>
#include <iostream>

namespace n_scheduler {

/**
 * Interface for all Scheduler instances.
 * @attention: standard priority is defined by:
 * std::less<T>(const T&, const T&) or operator<(const T&, const T&)const;
 *
 * The default implementation is a max heap, to use min heap reverse
 * the logic of the operator< or std::less<T>()
 *
 * The type T requires an implementation of std::hash<T>().
 *
 * Unschedule_until relies on a sane implementation of operator==()
 */
template<typename T, typename Time = T, typename TConstRef = const T&>
class Scheduler
{
protected:
	// Shield from use.
	Scheduler()
	{
		;
	}
	Scheduler(Scheduler&&) = default;
	Scheduler& operator=(const Scheduler&) = default;
	Scheduler(Scheduler&) = default;

public:
	/**
	 * @brief T_sched_item_type Provide users with explicit type of items being scheduled.
	 */
	typedef T t_value_type;

	virtual ~Scheduler()
	{
		;
	}

	/**
	 * @brief push Add item to scheduler.
	 * @attention : total order defined by result of operator<(const T&, const T&);
	 * @note : An item can be pushed with the same priority, but never with the same hash value twice.
	 * @throws bad_alloc in out of memory conditions.
	 */
	virtual void push_back(TConstRef) = 0;

	virtual size_t size() const = 0;

	virtual bool empty() const = 0;

	/**
	 * @brief top Retrieve first item in scheduler, do not remove it.
	 * This is not const, if T = pointer, access to const T& can
	 * invalidate the heap.
	 * @pre (not this.empty())
	 * @throws std::out_of_range if empty().
	 */
	virtual TConstRef top() = 0;

	/**
	 * @brief pop Remove the first entry in the scheduler.
	 * @pre (not this.empty())
	 * @throws std::out_of_range if empty().
	 * @post size-=1
	 */
	virtual T pop() = 0;

	/**
	 * @brief isLockable : gives an indication if the implementing subclass is synchronized or not. Note that single operation synchronization can be
	 * provided,  multiple operations (eg. size() && pop() are never safe without your own lock.
	 */
	virtual
	bool isLockable() const = 0;

	/**
	 * @brief unschedule_until Remove all items from the scheduler until their priority (time) exceeds the parameter.
	 * @param container Ordered result of popped items.
	 * @param time max priority/time value (included) of items to be removed from scheduler.
	 */
	virtual
	void
	unschedule_until(std::vector<T>& container, const Time& time) = 0;

	/**
	 * Remove all items from the scheduler.
	 */
	virtual
	void
	clear() = 0;

	/**
	 * Return true if the given element is present in the scheduler.
	 * @note O(1) (hashtable lookup).
	 */
	virtual
	bool
	contains(TConstRef elem) const= 0;

	/**
	 * Erase the element from the scheduler if present.
	 * @note O(log n)
	 * @return true if element was found and erased, false otherwise.
	 */
	virtual
	bool
	erase(TConstRef elem) = 0;

	virtual
	void
	printScheduler() const = 0;

	/**
	 * Test any invariant that has to hold during modifying operations.
	 */
	virtual
	void
	testInvariant() const = 0;
        
        /**
         * If item (key) is present in heap, overwrite it, else insert.
         * @param 
         */
        virtual
        void
        update(TConstRef){;}

         /**
          * Updates all present items
          */
        virtual
        void
        updateAll(){;}
        
        virtual
        void
        hintSize(size_t /*expected_size*/){;}
};

template<typename T, typename Time, typename TConstRef>
bool Scheduler<T, Time, TConstRef>::isLockable() const
{
	return false;
}

/**
 * @brief The ExampleItem struct
 * This is an example of a min heap item.
 * Any item that goes into the scheduler must have :
 *      Default constructor, copyconstructor & assignment operator.
 *      + Move versions.
 *      - no const members if you use the default versions.
 *      + an overload of:
 *      operator<
 *      operator==
 *      operator>       // define these two as not (the others)
 *      operator!=
 *	And a hash<T> implementation in namespace std, see below.
 *
 */
struct ExampleItem
{
	typedef int timetype;

	timetype prior;

	ExampleItem()
		: prior(0)
	{
		;
	}
	ExampleItem(timetype p)
		: prior(p)
	{
		;
	}
	ExampleItem(const ExampleItem& rhs)
	{
		this->prior = rhs.prior;
	}

	bool operator<(const ExampleItem& right) const
	{
		return this->prior > right.prior;
	}

	bool operator>(const ExampleItem& right) const
	{
		return not (this->operator <(right));
	}

	bool operator==(const ExampleItem& right) const
	{
		return this->prior == right.prior;
	}

	friend
	std::ostream&
	operator<<(std::ostream& os, const ExampleItem& rhs){
		return (os << rhs.prior);
	}
};

} // ENamespace

// Reference std::hash<T> implementation for a user supplied class.
// Extending namespace std is the default way to implement this. Note that the operator MUST be const, and any deviation from size_t as return type is liable to break things.
// Last but not least : a == b => hash(a) == hash(b) but not the other way around.
namespace std {
template<>
struct hash<n_scheduler::ExampleItem>
{
	size_t operator()(const n_scheduler::ExampleItem& item) const
	{
		// Defer hash of item to hash of 1 member. To use many, look at boost's implementation of hash combine.
		return hash<n_scheduler::ExampleItem::timetype>()(item.prior);
	}
};
}

#endif // SCHEDULER_H
