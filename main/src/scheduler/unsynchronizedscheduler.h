/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#ifndef UNSYNCHRONIZEDSCHEDULER_H
#define UNSYNCHRONIZEDSCHEDULER_H

#include "scheduler/scheduler.h"
#include <mutex>
#include <unordered_map>
#include <map>
#include <sstream>

namespace n_scheduler {

// Forward declare friend
template<typename T>
class SchedulerFactory;

/**
 * Unlocked Scheduler.
 * Each item can be stored once (for equal hash values).
 * @see SchedulerFactory for construction.
 * @param X Storage type
 * @param S Item type
 */
template<typename X, typename S>
class UnSynchronizedScheduler: public Scheduler<S> {
public:
	typedef typename X::handle_type t_handle;
private:
	/**
	 * @brief m_storage Storage backend. Any form of heap.
	 */
	X m_storage;

	typedef std::unordered_map<S, t_handle> t_hashtable;
        //typedef std::map<S, t_handle> t_hashtable;

	t_hashtable m_hashtable;

	friend class SchedulerFactory<S> ;

protected:
	UnSynchronizedScheduler() {
		;
	}

public:

	virtual ~UnSynchronizedScheduler() {
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
#include <scheduler/unsynchronizedscheduler.tpp>
#endif
