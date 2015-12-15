/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#ifndef SYNCHRONIZEDSCHEDULER_H
#define SYNCHRONIZEDSCHEDULER_H

#include "scheduler/scheduler.h"
#include <mutex>
#include <unordered_map>
#include <sstream>

namespace n_scheduler {

// Forward declare friend
template<typename T>
class SchedulerFactory;

/**
 * @brief A synchronized Scheduler.
 * This class provides basic scheduling facilities, with a lock on
 * each member function.
 * This means that it is thread safe for single function calls, but has to
 * be externally locked in e.g.:
 * if(not empty())
 *      pop()
 * With 2-n threads, this can still fail.
 * The correct version:
 *
 * mutex mylock;
 * {   // Use anonymous block for RAII unlock
 *     std::lock_guard<mutex> lock(mylock);
 *     if(not s.empty()){
 *       s.pop();
 *     }
 * }// Lock_guard unlocks always here.
 * @see SchedulerFactory for construction.
 * @param X Storage type
 * @param S Item type
 */
template<typename X, typename S>
class SynchronizedScheduler: public Scheduler<S> {
public:
	typedef typename X::handle_type t_handle;
private:
	/**
	 * @brief m_storage Storage backend. Any form of heap.
	 */
	X m_storage;

	typedef std::unordered_map<S, t_handle> t_hashtable;

	t_hashtable m_hashtable;

	friend class SchedulerFactory<S> ;

	/**
	 * @brief m_lock Internal mutex.
	 */
	mutable std::mutex m_lock;

protected:
	SynchronizedScheduler() {
		;
	}

public:

	virtual ~SynchronizedScheduler() {
		;
	}

	/**
	 * @brief push Schedule item.
	 * @pre S has a fully ordered implementation of S::operator< or std::less<S>
	 * @post size += 1
	 */
	virtual void push_back(const S&) override;

	virtual size_t size() const override;

	/**
	 * @brief empty
	 * @attention : if you use empty to protect from
	 * popping empty scheduler, make sure both empty() and pop()
	 * are wrapped in a single lock.
	 * @return
	 */
	virtual bool empty() const override;

	/**
	 * @brief top Get item to be scheduled first.
	 * @pre size()>0
	 * @invariant size() == size()
	 * @throws out_of_range if scheduler is emtpy.
	 * @post invariant
	 * @return
	 */
	virtual const S& top() override;

	/**
	 * @brief pop Remove and return top item of scheduler.
	 * @pre size()>0
	 * @post size-=1
	 * @throws out_of_range if scheduler is emtpy.
	 * @return
	 */
	virtual S pop() override;

	/**
	 * @brief isLockable
	 * @return true, indicates instances is locked on single method call.
	 */
	virtual bool isLockable() const override;

	virtual
	void
	unschedule_until(std::vector<S> &container, const S& time) override;

	void
	clear() override;

	virtual
	bool
	contains(const S& elem) const override;

	virtual
	bool
	erase(const S& elem) override;

	virtual
	void
	printScheduler() const override;

	virtual
	void
	testInvariant() const override;

};

}
#include <scheduler/synchronizedscheduler.tpp>
#endif
